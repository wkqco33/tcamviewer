package tcamviewer

import (
	"bytes"
	"io"
	"strings"
	"testing"
)

func TestTerminalSize(t *testing.T) {
	cols, rows, err := GetTerminalSize()
	if err != nil {
		t.Fatalf("GetTerminalSize() failed: %v", err)
	}
	if cols <= 0 || rows <= 0 {
		t.Fatalf("expected positive dimensions, got %dx%d", cols, rows)
	}
}

func TestRendererLifecycle(t *testing.T) {
	renderer, err := NewRenderer(Config{
		TargetCols: 4,
		TargetRows: 2,
		UseDiff:    false,
		AltScreen:  false,
		HideCursor: false,
	})
	if err != nil {
		t.Fatalf("NewRenderer() failed: %v", err)
	}
	defer renderer.Close()

	if renderer.Cols() != 4 || renderer.Rows() != 2 {
		t.Fatalf("expected 4x2, got %dx%d", renderer.Cols(), renderer.Rows())
	}

	rgb := make([]byte, 4*4*3)
	for i := range rgb {
		rgb[i] = 200
	}

	out, err := renderer.RenderToBuffer(rgb, 4, 4, 12, false)
	if err != nil {
		t.Fatalf("RenderToBuffer() failed: %v", err)
	}
	if !strings.Contains(out, "▀") {
		t.Fatalf("expected ANSI string to contain half-block character, got: %s", out)
	}

	if err := renderer.Resize(8, 4); err != nil {
		t.Fatalf("Resize() failed: %v", err)
	}
	if renderer.Cols() != 8 || renderer.Rows() != 4 {
		t.Fatalf("expected 8x4, got %dx%d", renderer.Cols(), renderer.Rows())
	}
}

func TestDecoderInvalid(t *testing.T) {
	_, err := NewDecoder("/non/existent/file.mp4", false)
	if err == nil {
		t.Fatal("expected error opening non-existent file, got nil")
	}
}

// newTestDecoder는 픽스처 비디오(w*h, fps, frameCount개)를 만들고 디코더를 연다.
func newTestDecoder(t *testing.T, loop bool, width, height, fps, frameCount int) (*Decoder, string) {
	t.Helper()
	path := writeTestAVI(t, t.TempDir(), width, height, fps, frameCount)
	decoder, err := NewDecoder(path, loop)
	if err != nil {
		t.Fatalf("NewDecoder(%s) failed: %v", path, err)
	}
	t.Cleanup(decoder.Close)
	return decoder, path
}

func TestDecoderInfo(t *testing.T) {
	decoder, _ := newTestDecoder(t, false, 64, 48, 10, 5)

	w, h, fps, err := decoder.Info()
	if err != nil {
		t.Fatalf("Info() failed: %v", err)
	}
	if w != 64 || h != 48 {
		t.Fatalf("expected 64x48, got %dx%d", w, h)
	}
	if fps < 9 || fps > 11 {
		t.Fatalf("expected fps around 10, got %v", fps)
	}
}

func TestDecoderReadFrame(t *testing.T) {
	decoder, _ := newTestDecoder(t, false, 64, 48, 10, 5)

	frame, err := decoder.ReadFrame(32, 24)
	if err != nil {
		t.Fatalf("ReadFrame failed: %v", err)
	}
	if frame.Width != 32 || frame.Height != 24 {
		t.Fatalf("expected scaled 32x24, got %dx%d", frame.Width, frame.Height)
	}
	if frame.Stride != frame.Width*3 {
		t.Fatalf("expected stride %d for RGB24, got %d", frame.Width*3, frame.Stride)
	}
	if len(frame.Data) != frame.Stride*frame.Height {
		t.Fatalf("expected data length %d, got %d", frame.Stride*frame.Height, len(frame.Data))
	}

	// 픽스처 프레임은 B=128 그라디언트다. 디코드/JPEG 손실을 고려해 여유를 둔다.
	readPx := func(x, y int) (byte, byte, byte) {
		i := y*frame.Stride + x*3
		return frame.Data[i], frame.Data[i+1], frame.Data[i+2]
	}
	_, _, b := readPx(0, frame.Height-1)
	if b < 98 || b > 158 {
		t.Fatalf("expected B channel around 128 at (0,%d), got %d", frame.Height-1, b)
	}

	// 나머지 프레임을 모두 읽으면 4개 더 읽힌 후 EOF여야 한다.
	for i := 0; i < 4; i++ {
		if _, err := decoder.ReadFrame(0, 0); err != nil {
			t.Fatalf("ReadFrame #%d failed: %v", i+2, err)
		}
	}
	if _, err := decoder.ReadFrame(0, 0); err != io.EOF {
		t.Fatalf("expected io.EOF after last frame, got %v", err)
	}
}

func TestDecoderReadFrameNativeResolution(t *testing.T) {
	decoder, _ := newTestDecoder(t, false, 64, 48, 10, 5)

	frame, err := decoder.ReadFrame(0, 0)
	if err != nil {
		t.Fatalf("ReadFrame failed: %v", err)
	}
	if frame.Width != 64 || frame.Height != 48 {
		t.Fatalf("expected native 64x48, got %dx%d", frame.Width, frame.Height)
	}
}

func TestDecoderRewind(t *testing.T) {
	decoder, _ := newTestDecoder(t, false, 64, 48, 10, 5)

	first, err := decoder.ReadFrame(0, 0)
	if err != nil {
		t.Fatalf("first ReadFrame failed: %v", err)
	}
	for {
		_, err := decoder.ReadFrame(0, 0)
		if err == io.EOF {
			break
		}
		if err != nil {
			t.Fatalf("ReadFrame failed: %v", err)
		}
	}

	if err := decoder.Rewind(); err != nil {
		t.Fatalf("Rewind failed: %v", err)
	}
	again, err := decoder.ReadFrame(0, 0)
	if err != nil {
		t.Fatalf("ReadFrame after rewind failed: %v", err)
	}
	if !bytes.Equal(first.Data, again.Data) {
		t.Fatal("frame after rewind differs from first frame")
	}
}

func TestDecoderLoop(t *testing.T) {
	decoder, _ := newTestDecoder(t, true, 64, 48, 10, 5)

	// 5프레임 짜리를 12번 읽으면 두 바퀴 이상 돌아야 한다(loop=true).
	for i := 0; i < 12; i++ {
		if _, err := decoder.ReadFrame(0, 0); err != nil {
			t.Fatalf("ReadFrame #%d failed (loop should rewind): %v", i+1, err)
		}
	}
}

func TestDecoderReadFrameClosed(t *testing.T) {
	path := writeTestAVI(t, t.TempDir(), 64, 48, 10, 5)
	decoder, err := NewDecoder(path, false)
	if err != nil {
		t.Fatalf("NewDecoder failed: %v", err)
	}
	decoder.Close()

	if _, err := decoder.ReadFrame(0, 0); err == nil {
		t.Fatal("expected error from closed decoder, got nil")
	}
	if err := decoder.Rewind(); err == nil {
		t.Fatal("expected error from closed decoder, got nil")
	}
}

func TestRendererRotation(t *testing.T) {
	renderer, err := NewRenderer(Config{
		TargetCols: 2,
		TargetRows: 1,
		Rotation:   90,
	})
	if err != nil {
		t.Fatalf("NewRenderer failed: %v", err)
	}
	defer renderer.Close()

	if renderer.Rotation() != 90 {
		t.Fatalf("expected rotation 90, got %d", renderer.Rotation())
	}

	if err := renderer.SetRotation(180); err != nil {
		t.Fatalf("SetRotation failed: %v", err)
	}
	if renderer.Rotation() != 180 {
		t.Fatalf("expected rotation 180, got %d", renderer.Rotation())
	}
}

func TestRendererKeepAspectRatio(t *testing.T) {
	renderer, err := NewRenderer(Config{
		TargetCols:      10,
		TargetRows:      5,
		KeepAspectRatio: true,
	})
	if err != nil {
		t.Fatalf("NewRenderer failed: %v", err)
	}
	defer renderer.Close()

	if !renderer.KeepAspectRatio() {
		t.Fatal("expected KeepAspectRatio to be true")
	}

	if err := renderer.SetKeepAspectRatio(false); err != nil {
		t.Fatalf("SetKeepAspectRatio failed: %v", err)
	}
	if renderer.KeepAspectRatio() {
		t.Fatal("expected KeepAspectRatio to be false")
	}
}
