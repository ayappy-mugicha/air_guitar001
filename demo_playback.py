"""Canon-progression demo playback (pure state transitions, no I/O)."""

DEMO_CHORDS = ("c", "g", "am", "em", "f", "c", "f", "g", "open")
DEMO_STROKE_INTERVAL_MS = 500
DEMO_CHORD_EVERY_TICKS = 4  # 4 * 500ms = 2s


def new_demo_state():
    return {
        "on": False,
        "chord_index": 0,
        "stroke_paused": False,
        "next_stroke": 0,
        "ticks": 0,
    }


def start_demo(state=None):
    """Turn demo on and emit the first chord immediately.

    Returns (state, chord, stroke_or_none).
    """
    first = DEMO_CHORDS[0]
    paused = first == "open"
    new_state = {
        "on": True,
        "chord_index": 1 % len(DEMO_CHORDS),
        "stroke_paused": paused,
        "next_stroke": 0 if paused else 1,
        "ticks": 0,
    }
    stroke = None if paused else 0
    return new_state, first, stroke


def stop_demo(state=None):
    """Turn demo off. Caller should publish `open` and stop strokes.

    Returns (state, chord).
    """
    return new_demo_state(), "open"


def tick_demo(state):
    """Advance a 500ms clock tick while demo is on.

    Every 4 ticks, emit the next chord. Strokes are omitted while paused
    (the current code is `open`).

    Returns (state, chord_or_none, stroke_or_none).
    """
    if not state or not state.get("on"):
        return state, None, None

    ticks = state.get("ticks", 0) + 1
    new_state = dict(state)
    new_state["ticks"] = ticks
    chord = None

    if ticks % DEMO_CHORD_EVERY_TICKS == 0:
        idx = new_state.get("chord_index", 0) % len(DEMO_CHORDS)
        chord = DEMO_CHORDS[idx]
        new_state["chord_index"] = (idx + 1) % len(DEMO_CHORDS)
        new_state["stroke_paused"] = chord == "open"

    stroke = None
    if not new_state.get("stroke_paused"):
        stroke = new_state.get("next_stroke", 0)
        new_state["next_stroke"] = 1 - int(stroke)

    return new_state, chord, stroke


def note_code_sent(state, message):
    """Pause strokes once when `open` is published during demo."""
    if not state or not state.get("on"):
        return state
    new_state = dict(state)
    new_state["stroke_paused"] = message == "open"
    return new_state
