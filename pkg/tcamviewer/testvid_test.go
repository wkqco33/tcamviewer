package tcamviewer

import (
	"bytes"
	"encoding/binary"
	"image"
	"image/color"
	"image/jpeg"
	"os"
	"path/filepath"
	"testing"
)

// writeTestAVI는 MJPEG 코덱의 작은 AVI 파일을 생성해 경로를 반환한다.
// ffmpeg 실행 파일 없이도 디코더 테스트 픽스처를 만들 수 있도록
// image/jpeg 표준 라이브러리만으로 RIFF/AVI 컨테이너를 직접 구성한다.
//
// 프레임마다 가로로 이동하는 마커가 있어 프레임 진행을 데이터로 구분할 수 있다.
func writeTestAVI(t *testing.T, dir string, width, height, fps, frameCount int) string {
	t.Helper()

	if width%2 != 0 || height%2 != 0 {
		t.Fatalf("테스트 픽스처 크기는 짝수여야 한다: %dx%d", width, height)
	}

	// 각 프레임을 JPEG로 인코딩한다.
	jpegs := make([][]byte, frameCount)
	maxSize := 0
	for i := range jpegs {
		jpegs[i] = encodeTestFrame(width, height, frameCount, i)
		if len(jpegs[i]) > maxSize {
			maxSize = len(jpegs[i])
		}
	}

	var buf bytes.Buffer
	putStr := func(s string) { buf.WriteString(s) }
	putU32 := func(v uint32) { _ = binary.Write(&buf, binary.LittleEndian, v) }

	// --- 청크 페이로드 준비 ---
	avih := make([]byte, 56)                                        // MainAVIHeader
	binary.LittleEndian.PutUint32(avih[0:4], uint32(1_000_000/fps)) // dwMicroSecPerFrame
	binary.LittleEndian.PutUint32(avih[4:8], uint32(maxSize*fps))   // dwMaxBytesPerSec
	binary.LittleEndian.PutUint32(avih[12:16], 0x10)                // dwFlags: AVIF_HASINDEX
	binary.LittleEndian.PutUint32(avih[16:20], uint32(frameCount))  // dwTotalFrames
	binary.LittleEndian.PutUint32(avih[24:28], 1)                   // dwStreams
	binary.LittleEndian.PutUint32(avih[28:32], uint32(maxSize))     // dwSuggestedBufferSize
	binary.LittleEndian.PutUint32(avih[32:36], uint32(width))       // dwWidth
	binary.LittleEndian.PutUint32(avih[36:40], uint32(height))      // dwHeight

	strh := make([]byte, 56)                                       // AVISTREAMHEADER
	copy(strh[0:4], "vids")                                        // fccType
	copy(strh[4:8], "MJPG")                                        // fccHandler
	binary.LittleEndian.PutUint32(strh[20:24], 1)                  // dwScale
	binary.LittleEndian.PutUint32(strh[24:28], uint32(fps))        // dwRate
	binary.LittleEndian.PutUint32(strh[32:36], uint32(frameCount)) // dwLength
	binary.LittleEndian.PutUint32(strh[36:40], uint32(maxSize))    // dwSuggestedBufferSize
	binary.LittleEndian.PutUint16(strh[48:50], uint16(width))      // rcFrame.right
	binary.LittleEndian.PutUint16(strh[50:52], uint16(height))     // rcFrame.bottom

	strf := make([]byte, 40) // BITMAPINFOHEADER
	binary.LittleEndian.PutUint32(strf[0:4], 40)
	binary.LittleEndian.PutUint32(strf[4:8], uint32(width))
	binary.LittleEndian.PutUint32(strf[8:12], uint32(height))
	binary.LittleEndian.PutUint16(strf[12:14], 1)  // biPlanes
	binary.LittleEndian.PutUint16(strf[14:16], 24) // biBitCount
	copy(strf[16:20], "MJPG")                      // biCompression
	binary.LittleEndian.PutUint32(strf[20:24], uint32(width*height*3))

	// --- 크기 계산 ---
	chunkLen := func(payload int) int { return 8 + payload + payload%2 }
	strlSize := 4 + chunkLen(len(strh)) + chunkLen(len(strf)) // 'strl' + strh 청크 + strf 청크
	hdrlSize := 4 + chunkLen(len(avih)) + 8 + strlSize        // 'hdrl' + avih 청크 + strl 리스트 청크
	moviSize := 4                                             // 'movi' 4CC
	for _, j := range jpegs {
		moviSize += chunkLen(len(j))
	}
	idxSize := 16 * frameCount
	riffPayload := 4 + chunkLen(hdrlSize) + chunkLen(moviSize) + chunkLen(idxSize)

	// --- RIFF 헤더 ---
	putStr("RIFF")
	putU32(uint32(riffPayload))
	putStr("AVI ")

	// --- hdrl 리스트 ---
	putStr("LIST")
	putU32(uint32(hdrlSize))
	putStr("hdrl")
	putStr("avih")
	putU32(uint32(len(avih)))
	buf.Write(avih)
	putStr("LIST")
	putU32(uint32(strlSize))
	putStr("strl")
	putStr("strh")
	putU32(uint32(len(strh)))
	buf.Write(strh)
	putStr("strf")
	putU32(uint32(len(strf)))
	buf.Write(strf)

	// --- movi 리스트 ---
	// idx1의 dwOffset은 'movi' 4CC 위치 기준이며, 첫 청크는 오프셋 4에서 시작한다.
	putStr("LIST")
	putU32(uint32(moviSize))
	putStr("movi")
	offsets := make([]uint32, frameCount)
	for i, j := range jpegs {
		offsets[i] = uint32(moviOffsetBefore(jpegs, i))
		putStr("00dc")
		putU32(uint32(len(j)))
		buf.Write(j)
		if len(j)%2 == 1 {
			buf.WriteByte(0) // 청크는 2바이트 정렬
		}
	}

	// --- idx1 인덱스 ---
	putStr("idx1")
	putU32(uint32(idxSize))
	for i := range jpegs {
		putStr("00dc")
		putU32(0x10) // AVIIF_KEYFRAME
		putU32(offsets[i])
		putU32(uint32(len(jpegs[i])))
	}

	if got := buf.Len(); got != 8+riffPayload {
		t.Fatalf("AVI 크기 계산 불일치: %d != %d", got, 8+riffPayload)
	}

	path := filepath.Join(dir, "test_video.avi")
	if err := os.WriteFile(path, buf.Bytes(), 0o644); err != nil {
		t.Fatalf("픽스처 쓰기 실패: %v", err)
	}
	return path
}

// moviOffsetBefore는 i번째 00dc 청크의 movi 기준 오프셋이다.
// 'movi' 4CC 바로 뒤(오프셋 4)에 첫 청크가 시작된다.
func moviOffsetBefore(jpegs [][]byte, i int) int {
	off := 4
	for _, j := range jpegs[:i] {
		off += 8 + len(j) + len(j)%2
	}
	return off
}

// encodeTestFrame은 프레임 번호에 따라 마커가 이동하는 그라디언트 프레임을
// JPEG로 인코딩한다.
func encodeTestFrame(width, height, total, idx int) []byte {
	img := image.NewRGBA(image.Rect(0, 0, width, height))
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			img.SetRGBA(x, y, color.RGBA{
				R: uint8(x * 255 / width),
				G: uint8(y * 255 / height),
				B: 128,
				A: 255,
			})
		}
	}
	// 프레임 진행에 따라 가로로 이동하는 마커
	mx := width * idx / total
	for dy := 0; dy < height/4; dy++ {
		for dx := 0; dx < width/4; dx++ {
			img.SetRGBA(mx+dx, dy, color.RGBA{R: 255, G: 255, B: 0, A: 255})
		}
	}
	var b bytes.Buffer
	if err := jpeg.Encode(&b, img, &jpeg.Options{Quality: 90}); err != nil {
		panic(err) // 표준 라이브러리 인코더 실패는 테스트 코드 버그다
	}
	return b.Bytes()
}
