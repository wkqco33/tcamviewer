#!/usr/bin/env python3
import sys
import os
import time
import math

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../python")))

from tcamviewer import TerminalRenderer, get_terminal_size

def main():
    cols, rows = get_terminal_size()
    print(f"Terminal size detected: {cols} cols, {rows} rows")

    renderer = TerminalRenderer(
        cols=cols,
        rows=rows,
        use_diff=True,
        alt_screen=True,
        hide_cursor=True
    )

    pixel_w = cols
    pixel_h = rows * 2

    print("Running Python Terminal Video Demo (press Ctrl+C to stop)...")
    try:
        for frame in range(60): # 2 seconds at 30fps
            start = time.time()
            t = frame * 0.08
            buf = bytearray(pixel_w * pixel_h * 3)

            for y in range(pixel_h):
                for x in range(pixel_w):
                    nx = x / pixel_w
                    ny = y / pixel_h
                    r = int((math.sin(nx * math.pi * 2.0 + t) * 0.5 + 0.5) * 255)
                    g = int((math.sin(ny * math.pi * 2.0 + t * 1.5) * 0.5 + 0.5) * 255)
                    b = int((math.cos((nx + ny) * math.pi + t * 0.7) * 0.5 + 0.5) * 255)

                    idx = (y * pixel_w + x) * 3
                    buf[idx + 0] = r
                    buf[idx + 1] = g
                    buf[idx + 2] = b

            renderer.render_rgb(bytes(buf), pixel_w, pixel_h, pixel_w * 3)
            elapsed = time.time() - start
            if elapsed < 0.033:
                time.sleep(0.033 - elapsed)
    except KeyboardInterrupt:
        pass
    finally:
        renderer.close()
        print("Demo completed.")

if __name__ == "__main__":
    main()
