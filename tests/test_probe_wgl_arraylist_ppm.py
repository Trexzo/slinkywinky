"""Synthetic P6 fixtures for the CI-only WGL pixel reporter."""
import tempfile
import unittest
from pathlib import Path

from tools.probe_wgl_arraylist_ppm import inspect_ppm, sample_gl


class WglPixelProbeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / "test.ppm"

    def test_bottom_left_opengl_coordinates(self):
        # PPM begins with top-left row while glReadPixels uses bottom-left.
        self.path.write_bytes(b"P6\n2 2\n255\n" + bytes([
            1, 2, 3, 4, 5, 6,
            7, 8, 9, 10, 11, 12,
        ]))
        w, h, pixels = inspect_ppm(self.path)
        self.assertEqual(sample_gl(w, h, pixels, 0, 0), (7, 8, 9))
        self.assertEqual(sample_gl(w, h, pixels, 1, 1), (4, 5, 6))
        with self.assertRaises(ValueError):
            sample_gl(w, h, pixels, 2, 0)

    def test_rejects_truncated_or_wrong_magic(self):
        self.path.write_bytes(b"P6\n2 2\n255\n" + b"\x00" * 11)
        with self.assertRaisesRegex(ValueError, "payload length"):
            inspect_ppm(self.path)
        self.path.write_bytes(b"P3\n2 2\n255\n" + b"\x00" * 12)
        with self.assertRaisesRegex(ValueError, "P6"):
            inspect_ppm(self.path)

    def test_rejects_invalid_dimensions(self):
        self.path.write_bytes(b"P6\n100000 2\n255\n")
        with self.assertRaisesRegex(ValueError, "bounds"):
            inspect_ppm(self.path)


if __name__ == "__main__":
    unittest.main()
