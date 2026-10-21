import unittest

from chord_hold import OPEN_CODE, apply_hold_event, interrupt_demo_on_press


class ChordHoldTest(unittest.TestCase):
    def test_press_publishes_chord(self):
        held, publish = apply_hold_event([], "press", "c")
        self.assertEqual(held, ["c"])
        self.assertEqual(publish, "c")

    def test_release_publishes_open(self):
        held, publish = apply_hold_event(["c"], "release", "c")
        self.assertEqual(held, [])
        self.assertEqual(publish, OPEN_CODE)

    def test_switch_chords_without_open(self):
        held, publish = apply_hold_event(["c"], "press", "g")
        self.assertEqual(held, ["c", "g"])
        self.assertEqual(publish, "g")

    def test_release_active_restores_previous(self):
        held, publish = apply_hold_event(["c", "g"], "release", "g")
        self.assertEqual(held, ["c"])
        self.assertEqual(publish, "c")

    def test_release_inactive_sends_nothing(self):
        held, publish = apply_hold_event(["c", "g"], "release", "c")
        self.assertEqual(held, ["g"])
        self.assertIsNone(publish)

    def test_double_release_sends_nothing(self):
        held, publish = apply_hold_event([], "release", "c")
        self.assertEqual(held, [])
        self.assertIsNone(publish)

    def test_repress_moves_to_active(self):
        held, publish = apply_hold_event(["c", "g"], "press", "c")
        self.assertEqual(held, ["g", "c"])
        self.assertEqual(publish, "c")

    def test_unknown_action_is_noop(self):
        held, publish = apply_hold_event(["c"], "cancel", "c")
        self.assertEqual(held, ["c"])
        self.assertIsNone(publish)

    def test_interrupt_demo_only_on_press(self):
        self.assertTrue(interrupt_demo_on_press(True, "press"))
        self.assertFalse(interrupt_demo_on_press(True, "release"))
        self.assertFalse(interrupt_demo_on_press(False, "press"))


if __name__ == "__main__":
    unittest.main()
