import unittest
from pathlib import Path

from guide_copy import GUIDE_SHEET_TITLE, GUIDE_STEPS, GUIDE_WARNING
from render_guide_sheet import render_sheet_html, write_sheet


class GuideCopyTest(unittest.TestCase):
    def test_five_casual_steps_and_warning(self):
        self.assertEqual(len(GUIDE_STEPS), 5)
        self.assertTrue(GUIDE_STEPS[2]["media"])
        self.assertIn("ジャラーン", GUIDE_STEPS[2]["body"])
        self.assertIn("ひとり", GUIDE_WARNING)

    def test_sheet_html_is_landscape_and_uses_copy(self):
        html = render_sheet_html()
        self.assertIn("A4 landscape", html)
        self.assertIn(GUIDE_SHEET_TITLE, html)
        self.assertIn(GUIDE_WARNING, html)
        for step in GUIDE_STEPS:
            self.assertIn(step["title"], html)
            self.assertIn(step["body"], html)
        self.assertIn("右手がここに映る", html)
        self.assertIn("stroke-art", html)

    def test_write_sheet_roundtrip(self):
        path = write_sheet()
        self.assertEqual(path, Path(__file__).resolve().parent / "assets" / "guide-sheet.html")
        self.assertTrue(path.exists())


if __name__ == "__main__":
    unittest.main()
