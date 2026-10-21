"""Hold-to-fret state: last pressed chord is active; open only when empty."""

OPEN_CODE = "open"


def apply_hold_event(held, action, chord_id):
    """Apply a press or release to the held-chord stack.

    ``held`` is press-order (last item is the sounding chord).
    Returns ``(new_held, publish_or_none)``.
    """
    if not chord_id or action not in ("press", "release"):
        return list(held or []), None

    stack = [item for item in (held or []) if item]
    if action == "press":
        if chord_id in stack:
            stack.remove(chord_id)
        stack.append(chord_id)
        return stack, chord_id

    if chord_id not in stack:
        return stack, None

    was_active = stack[-1] == chord_id
    stack.remove(chord_id)
    if not stack:
        return stack, OPEN_CODE
    if was_active:
        return stack, stack[-1]
    return stack, None


def interrupt_demo_on_press(demo_on, action):
    """True when a chord press should stop automatic demo playback."""
    return bool(demo_on) and action == "press"
