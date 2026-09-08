package tcamviewer

import (
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
