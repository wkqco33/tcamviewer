import ctypes
import os
import sys
from typing import Tuple, Optional

# Locate libtcamviewer.so
def _load_library() -> ctypes.CDLL:
    search_paths = [
        os.path.join(os.path.dirname(__file__), "../../build/libtcamviewer.so"),
        os.path.join(os.path.dirname(__file__), "../../../build/libtcamviewer.so"),
        os.path.join(os.path.dirname(__file__), "libtcamviewer.so"),
        "/usr/local/lib/libtcamviewer.so",
        "/usr/lib/libtcamviewer.so",
    ]
    for path in search_paths:
        abs_path = os.path.abspath(path)
        if os.path.exists(abs_path):
            return ctypes.CDLL(abs_path)
    # Try system library loader
    try:
        return ctypes.CDLL("libtcamviewer.so")
    except OSError as e:
        raise RuntimeError(
            f"Failed to find libtcamviewer.so. Please build the project first (mkdir build && cd build && cmake .. && make)."
        ) from e

_lib = _load_library()

class _RenderConfig(ctypes.Structure):
    _fields_ = [
        ("target_cols", ctypes.c_int),
        ("target_rows", ctypes.c_int),
        ("use_diff", ctypes.c_bool),
        ("alt_screen", ctypes.c_bool),
        ("hide_cursor", ctypes.c_bool),
        ("rotation", ctypes.c_int),
        ("keep_aspect_ratio", ctypes.c_bool),
    ]

# Function signatures
_lib.tcam_get_terminal_size.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
_lib.tcam_get_terminal_size.restype = ctypes.c_int

_lib.tcam_renderer_create.argtypes = [ctypes.POINTER(_RenderConfig)]
_lib.tcam_renderer_create.restype = ctypes.c_void_p

_lib.tcam_renderer_destroy.argtypes = [ctypes.c_void_p]
_lib.tcam_renderer_destroy.restype = None

_lib.tcam_renderer_resize.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
_lib.tcam_renderer_resize.restype = ctypes.c_int

_lib.tcam_renderer_set_rotation.argtypes = [ctypes.c_void_p, ctypes.c_int]
_lib.tcam_renderer_set_rotation.restype = ctypes.c_int

_lib.tcam_renderer_get_rotation.argtypes = [ctypes.c_void_p]
_lib.tcam_renderer_get_rotation.restype = ctypes.c_int

_lib.tcam_renderer_set_keep_aspect_ratio.argtypes = [ctypes.c_void_p, ctypes.c_bool]
_lib.tcam_renderer_set_keep_aspect_ratio.restype = ctypes.c_int

_lib.tcam_renderer_get_keep_aspect_ratio.argtypes = [ctypes.c_void_p]
_lib.tcam_renderer_get_keep_aspect_ratio.restype = ctypes.c_bool

_lib.tcam_renderer_render_rgb24.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
_lib.tcam_renderer_render_rgb24.restype = ctypes.c_int

_lib.tcam_renderer_render_bgr24.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
_lib.tcam_renderer_render_bgr24.restype = ctypes.c_int

_lib.tcam_renderer_render_to_buffer.argtypes = [
    ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int,
    ctypes.c_bool, ctypes.c_char_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)
]
_lib.tcam_renderer_render_to_buffer.restype = ctypes.c_int

_lib.tcam_renderer_invalidate_cache.argtypes = [ctypes.c_void_p]
_lib.tcam_renderer_invalidate_cache.restype = None

_lib.tcam_renderer_get_cols.argtypes = [ctypes.c_void_p]
_lib.tcam_renderer_get_cols.restype = ctypes.c_int

_lib.tcam_renderer_get_rows.argtypes = [ctypes.c_void_p]
_lib.tcam_renderer_get_rows.restype = ctypes.c_int


def get_terminal_size() -> Tuple[int, int]:
    cols = ctypes.c_int()
    rows = ctypes.c_int()
    status = _lib.tcam_get_terminal_size(ctypes.byref(cols), ctypes.byref(rows))
    if status != 0:
        return 80, 24
    return cols.value, rows.value


class TerminalRenderer:
    def __init__(self, cols: int = 0, rows: int = 0, use_diff: bool = True,
                 alt_screen: bool = False, hide_cursor: bool = True, rotation: int = 0,
                 keep_aspect_ratio: bool = True):
        cfg = _RenderConfig(
            target_cols=cols,
            target_rows=rows,
            use_diff=use_diff,
            alt_screen=alt_screen,
            hide_cursor=hide_cursor,
            rotation=rotation,
            keep_aspect_ratio=keep_aspect_ratio
        )
        self._ptr = _lib.tcam_renderer_create(ctypes.byref(cfg))
        if not self._ptr:
            raise RuntimeError("Failed to create native tcam renderer")

    def __del__(self):
        self.close()

    def close(self):
        if hasattr(self, "_ptr") and self._ptr:
            _lib.tcam_renderer_destroy(self._ptr)
            self._ptr = None

    def resize(self, cols: int, rows: int):
        if not self._ptr:
            raise RuntimeError("Renderer is closed")
        status = _lib.tcam_renderer_resize(self._ptr, cols, rows)
        if status != 0:
            raise RuntimeError(f"Failed to resize renderer, status: {status}")

    def set_rotation(self, degrees: int):
        if not self._ptr:
            raise RuntimeError("Renderer is closed")
        status = _lib.tcam_renderer_set_rotation(self._ptr, degrees)
        if status != 0:
            raise RuntimeError(f"Failed to set rotation, status: {status}")

    def set_keep_aspect_ratio(self, enable: bool):
        if not self._ptr:
            raise RuntimeError("Renderer is closed")
        status = _lib.tcam_renderer_set_keep_aspect_ratio(self._ptr, enable)
        if status != 0:
            raise RuntimeError(f"Failed to set keep_aspect_ratio, status: {status}")

    def invalidate_cache(self):
        if self._ptr:
            _lib.tcam_renderer_invalidate_cache(self._ptr)

    @property
    def cols(self) -> int:
        return _lib.tcam_renderer_get_cols(self._ptr) if self._ptr else 0

    @property
    def rows(self) -> int:
        return _lib.tcam_renderer_get_rows(self._ptr) if self._ptr else 0

    @property
    def rotation(self) -> int:
        return _lib.tcam_renderer_get_rotation(self._ptr) if self._ptr else 0

    @property
    def keep_aspect_ratio(self) -> bool:
        return _lib.tcam_renderer_get_keep_aspect_ratio(self._ptr) if self._ptr else True

    def _to_bytes_pointer(self, data):
        if isinstance(data, bytes):
            return data
        if hasattr(data, "ctypes"):
            # Numpy ndarray
            return ctypes.cast(data.ctypes.data, ctypes.c_char_p)
        if isinstance(data, bytearray):
            return bytes(data)
        raise TypeError("Expected bytes or numpy ndarray")

    def render_rgb(self, data, width: int, height: int, stride: int = 0):
        if not self._ptr:
            raise RuntimeError("Renderer is closed")
        ptr = self._to_bytes_pointer(data)
        status = _lib.tcam_renderer_render_rgb24(self._ptr, ptr, width, height, stride)
        if status != 0:
            raise RuntimeError(f"render_rgb failed, status: {status}")

    def render_bgr(self, data, width: int, height: int, stride: int = 0):
        if not self._ptr:
            raise RuntimeError("Renderer is closed")
        ptr = self._to_bytes_pointer(data)
        status = _lib.tcam_renderer_render_bgr24(self._ptr, ptr, width, height, stride)
        if status != 0:
            raise RuntimeError(f"render_bgr failed, status: {status}")

    def render_to_buffer(self, data, width: int, height: int, stride: int = 0, is_bgr: bool = False) -> str:
        if not self._ptr:
            raise RuntimeError("Renderer is closed")
        ptr = self._to_bytes_pointer(data)
        buf_size = width * height * 30 + 4096
        out_buf = ctypes.create_string_buffer(buf_size)
        out_len = ctypes.c_size_t()
        status = _lib.tcam_renderer_render_to_buffer(
            self._ptr, ptr, width, height, stride, is_bgr, out_buf, buf_size, ctypes.byref(out_len)
        )
        if status != 0:
            raise RuntimeError(f"render_to_buffer failed, status: {status}")
        return out_buf.value.decode("utf-8", errors="replace")
