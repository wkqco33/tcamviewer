package tcamviewer

/*
#cgo CFLAGS: -I${SRCDIR}/../../include
#cgo LDFLAGS: -L${SRCDIR}/../../build -ltcamviewer -Wl,-rpath,${SRCDIR}/../../build
#include "tcamviewer/tcamviewer.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"fmt"
	"unsafe"
)

type Config struct {
	TargetCols int
	TargetRows int
	UseDiff    bool
	AltScreen  bool
	HideCursor bool
	Rotation   int // 0, 90, 180, 270 (Clockwise)
}

type Renderer struct {
	ptr *C.tcam_renderer_t
}

func GetTerminalSize() (cols, rows int, err error) {
	var cCols, cRows C.int
	status := C.tcam_get_terminal_size(&cCols, &cRows)
	if status != C.TCAM_OK {
		return 0, 0, fmt.Errorf("failed to get terminal size, status code: %d", int(status))
	}
	return int(cCols), int(cRows), nil
}

func NewRenderer(cfg Config) (*Renderer, error) {
	cCfg := C.tcam_render_config_t{
		target_cols: C.int(cfg.TargetCols),
		target_rows: C.int(cfg.TargetRows),
		use_diff:    C.bool(cfg.UseDiff),
		alt_screen:  C.bool(cfg.AltScreen),
		hide_cursor: C.bool(cfg.HideCursor),
		rotation:    C.int(cfg.Rotation),
	}

	ptr := C.tcam_renderer_create(&cCfg)
	if ptr == nil {
		return nil, errors.New("failed to create tcam renderer")
	}

	return &Renderer{ptr: ptr}, nil
}

func (r *Renderer) Close() {
	if r != nil && r.ptr != nil {
		C.tcam_renderer_destroy(r.ptr)
		r.ptr = nil
	}
}

func (r *Renderer) Resize(cols, rows int) error {
	if r == nil || r.ptr == nil {
		return errors.New("renderer is nil or closed")
	}
	st := C.tcam_renderer_resize(r.ptr, C.int(cols), C.int(rows))
	if st != C.TCAM_OK {
		return fmt.Errorf("resize failed with status %d", int(st))
	}
	return nil
}

func (r *Renderer) InvalidateCache() {
	if r != nil && r.ptr != nil {
		C.tcam_renderer_invalidate_cache(r.ptr)
	}
}

func (r *Renderer) Cols() int {
	if r == nil || r.ptr == nil {
		return 0
	}
	return int(C.tcam_renderer_get_cols(r.ptr))
}

func (r *Renderer) Rows() int {
	if r == nil || r.ptr == nil {
		return 0
	}
	return int(C.tcam_renderer_get_rows(r.ptr))
}

func (r *Renderer) Rotation() int {
	if r == nil || r.ptr == nil {
		return 0
	}
	return int(C.tcam_renderer_get_rotation(r.ptr))
}

func (r *Renderer) SetRotation(degrees int) error {
	if r == nil || r.ptr == nil {
		return errors.New("renderer is nil or closed")
	}
	st := C.tcam_renderer_set_rotation(r.ptr, C.int(degrees))
	if st != C.TCAM_OK {
		return fmt.Errorf("set_rotation failed with status %d", int(st))
	}
	return nil
}

func (r *Renderer) RenderRGB(rgb []byte, width, height, stride int) error {
	if r == nil || r.ptr == nil {
		return errors.New("renderer is nil or closed")
	}
	if len(rgb) == 0 {
		return errors.New("empty RGB data")
	}

	st := C.tcam_renderer_render_rgb24(
		r.ptr,
		(*C.uint8_t)(unsafe.Pointer(&rgb[0])),
		C.int(width),
		C.int(height),
		C.int(stride),
	)
	if st != C.TCAM_OK {
		return fmt.Errorf("render_rgb failed with status %d", int(st))
	}
	return nil
}

func (r *Renderer) RenderBGR(bgr []byte, width, height, stride int) error {
	if r == nil || r.ptr == nil {
		return errors.New("renderer is nil or closed")
	}
	if len(bgr) == 0 {
		return errors.New("empty BGR data")
	}

	st := C.tcam_renderer_render_bgr24(
		r.ptr,
		(*C.uint8_t)(unsafe.Pointer(&bgr[0])),
		C.int(width),
		C.int(height),
		C.int(stride),
	)
	if st != C.TCAM_OK {
		return fmt.Errorf("render_bgr failed with status %d", int(st))
	}
	return nil
}

func (r *Renderer) RenderToBuffer(data []byte, width, height, stride int, isBGR bool) (string, error) {
	if r == nil || r.ptr == nil {
		return "", errors.New("renderer is nil or closed")
	}
	if len(data) == 0 {
		return "", errors.New("empty data")
	}

	bufSize := (width * height * 30) + 1024
	buf := make([]byte, bufSize)
	var outLen C.size_t

	st := C.tcam_renderer_render_to_buffer(
		r.ptr,
		(*C.uint8_t)(unsafe.Pointer(&data[0])),
		C.int(width),
		C.int(height),
		C.int(stride),
		C.bool(isBGR),
		(*C.char)(unsafe.Pointer(&buf[0])),
		C.size_t(bufSize),
		&outLen,
	)
	if st != C.TCAM_OK {
		return "", fmt.Errorf("render_to_buffer failed with status %d", int(st))
	}

	return string(buf[:outLen]), nil
}

type Decoder struct {
	ptr *C.tcam_decoder_t
}

func NewDecoder(source string, loop bool) (*Decoder, error) {
	cSrc := C.CString(source)
	defer C.free(unsafe.Pointer(cSrc))

	ptr := C.tcam_decoder_create(cSrc, C.bool(loop))
	if ptr == nil {
		return nil, fmt.Errorf("failed to open video source: %s", source)
	}

	return &Decoder{ptr: ptr}, nil
}

func (d *Decoder) Close() {
	if d != nil && d.ptr != nil {
		C.tcam_decoder_destroy(d.ptr)
		d.ptr = nil
	}
}

func (d *Decoder) Info() (width, height int, fps float64, err error) {
	if d == nil || d.ptr == nil {
		return 0, 0, 0, errors.New("decoder is nil or closed")
	}
	var w, h C.int
	var f C.double
	st := C.tcam_decoder_get_info(d.ptr, &w, &h, &f)
	if st != C.TCAM_OK {
		return 0, 0, 0, fmt.Errorf("failed to get info, status: %d", int(st))
	}
	return int(w), int(h), float64(f), nil
}

func (d *Decoder) Rotation() int {
	if d == nil || d.ptr == nil {
		return 0
	}
	return int(C.tcam_decoder_get_rotation(d.ptr))
}
