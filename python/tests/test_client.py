import unittest
import sys
import os

# Add python directory to sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from tcamviewer import TerminalRenderer, get_terminal_size

class TestTcamviewerPython(unittest.TestCase):
    def test_terminal_size(self):
        cols, rows = get_terminal_size()
        self.assertGreater(cols, 0)
        self.assertGreater(rows, 0)

    def test_renderer_lifecycle_and_buffer(self):
        renderer = TerminalRenderer(cols=4, rows=2, use_diff=False, alt_screen=False, hide_cursor=False)
        self.assertEqual(renderer.cols, 4)
        self.assertEqual(renderer.rows, 2)

        # 4 cols x 2 rows -> 4x4 image
        rgb = bytes([128] * (4 * 4 * 3))
        ansi = renderer.render_to_buffer(rgb, 4, 4, 12, is_bgr=False)
        self.assertTrue(len(ansi) > 0)
        self.assertIn("\u2580", ansi) # Half block '▀'

        renderer.resize(6, 3)
        self.assertEqual(renderer.cols, 6)
        self.assertEqual(renderer.rows, 3)
        renderer.close()

if __name__ == "__main__":
    unittest.main()
