import unittest

from demo_playback import (
    DEMO_CHORDS,
    DEMO_CHORD_EVERY_TICKS,
    new_demo_state,
    note_code_sent,
    start_demo,
    stop_demo,
    tick_demo,
)


class DemoPlaybackTest(unittest.TestCase):
    def test_sequence_loops_with_open(self):
        state, first, stroke = start_demo(new_demo_state())
        self.assertEqual(first, "c")
        self.assertEqual(stroke, 0)
        chords = [first]
        while len(chords) < len(DEMO_CHORDS):
            state, chord, _ = tick_demo(state)
            if chord is not None:
                chords.append(chord)
        self.assertEqual(tuple(chords), DEMO_CHORDS)
        for _ in range(DEMO_CHORD_EVERY_TICKS):
            state, chord, _ = tick_demo(state)
            if chord is not None:
                break
        self.assertEqual(chord, "c")

    def test_open_pauses_stroke_then_resumes(self):
        state, _, _ = start_demo(new_demo_state())
        chord = None
        while chord != "open":
            state, chord, stroke = tick_demo(state)
        self.assertTrue(state["stroke_paused"])
        self.assertIsNone(stroke)
        state, stroke_while_open = _next_stroke(state)
        self.assertIsNone(stroke_while_open)
        state, chord, _ = _next_chord(state)
        self.assertEqual(chord, "c")
        self.assertFalse(state["stroke_paused"])
        state, resumed = _next_stroke(state)
        self.assertIn(resumed, (0, 1))

    def test_full_open_pauses_stroke_once(self):
        state, _, _ = start_demo(new_demo_state())
        state, _, stroke = tick_demo(state)
        self.assertIn(stroke, (0, 1))
        state = note_code_sent(state, "open")
        self.assertTrue(state["stroke_paused"])
        state, paused_stroke = _next_stroke(state)
        self.assertIsNone(paused_stroke)
        state, chord, _ = _next_chord(state)
        self.assertEqual(chord, "g")
        self.assertFalse(state["stroke_paused"])

    def test_stop_sends_open_and_clears_on(self):
        state, _, _ = start_demo(new_demo_state())
        state, chord = stop_demo(state)
        self.assertFalse(state["on"])
        self.assertEqual(chord, "open")
        state, next_chord, next_stroke = tick_demo(state)
        self.assertIsNone(next_chord)
        self.assertIsNone(next_stroke)

    def test_strokes_alternate_while_playing(self):
        state, _, first_stroke = start_demo(new_demo_state())
        strokes = [first_stroke]
        for _ in range(3):
            state, _, stroke = tick_demo(state)
            strokes.append(stroke)
        self.assertEqual(strokes, [0, 1, 0, 1])


def _next_chord(state):
    chord = None
    stroke = None
    while chord is None:
        state, chord, stroke = tick_demo(state)
    return state, chord, stroke


def _next_stroke(state):
    """One clock tick; used to assert pause (None) vs a stroke value."""
    state, _, stroke = tick_demo(state)
    return state, stroke


if __name__ == "__main__":
    unittest.main()
