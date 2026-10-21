(function () {
    var queue = [];
    var keysDown = Object.create(null);
    var pointersDown = Object.create(null);

    function emit(action, chordId) {
        if (!chordId || (action !== "press" && action !== "release")) {
            return;
        }
        var payload = { action: action, id: chordId, t: Date.now() };
        if (
            window.dash_clientside &&
            typeof window.dash_clientside.set_props === "function"
        ) {
            try {
                window.dash_clientside.set_props("hold-event", { data: payload });
                return;
            } catch (err) {
                queue.push(payload);
                return;
            }
        }
        queue.push(payload);
    }

    function flushQueue() {
        if (
            !queue.length ||
            !window.dash_clientside ||
            typeof window.dash_clientside.set_props !== "function"
        ) {
            return;
        }
        while (queue.length) {
            window.dash_clientside.set_props("hold-event", { data: queue.shift() });
        }
    }

    function padFromEvent(event) {
        var node = event.target;
        if (!node || !node.closest) {
            return null;
        }
        return node.closest(".chord-pad");
    }

    function setHeld(pad, held) {
        if (!pad) {
            return;
        }
        pad.classList.toggle("is-held", held);
        pad.setAttribute("aria-pressed", held ? "true" : "false");
    }

    function onPointerDown(event) {
        var pad = padFromEvent(event);
        if (!pad || pad.disabled) {
            return;
        }
        if (event.pointerType === "mouse" && event.button !== 0) {
            return;
        }
        event.preventDefault();
        if (pointersDown[event.pointerId]) {
            return;
        }
        try {
            pad.setPointerCapture(event.pointerId);
        } catch (err) {
            /* capture is best-effort on older browsers */
        }
        pointersDown[event.pointerId] = pad;
        setHeld(pad, true);
        emit("press", pad.getAttribute("data-chord"));
    }

    function onPointerEnd(event) {
        var pad = pointersDown[event.pointerId] || padFromEvent(event);
        if (!pad || !pointersDown[event.pointerId]) {
            return;
        }
        delete pointersDown[event.pointerId];
        if (
            pad.hasPointerCapture &&
            event.pointerId != null &&
            pad.hasPointerCapture(event.pointerId)
        ) {
            try {
                pad.releasePointerCapture(event.pointerId);
            } catch (err) {
                /* already released */
            }
        }
        setHeld(pad, false);
        emit("release", pad.getAttribute("data-chord"));
    }

    function isHoldKey(event) {
        return event.key === " " || event.key === "Enter";
    }

    function onKeyDown(event) {
        var pad = padFromEvent(event);
        if (!pad || pad.disabled || !isHoldKey(event) || event.repeat) {
            return;
        }
        event.preventDefault();
        var chordId = pad.getAttribute("data-chord");
        if (keysDown[chordId]) {
            return;
        }
        keysDown[chordId] = true;
        setHeld(pad, true);
        emit("press", chordId);
    }

    function onKeyUp(event) {
        var pad = padFromEvent(event);
        if (!pad || !isHoldKey(event)) {
            return;
        }
        event.preventDefault();
        var chordId = pad.getAttribute("data-chord");
        if (!keysDown[chordId]) {
            return;
        }
        keysDown[chordId] = false;
        setHeld(pad, false);
        emit("release", chordId);
    }

    document.addEventListener("pointerdown", onPointerDown, { capture: true });
    document.addEventListener("pointerup", onPointerEnd, { capture: true });
    document.addEventListener("pointercancel", onPointerEnd, { capture: true });
    document.addEventListener("lostpointercapture", onPointerEnd, { capture: true });
    document.addEventListener("keydown", onKeyDown, { capture: true });
    document.addEventListener("keyup", onKeyUp, { capture: true });
    document.addEventListener("contextmenu", function (event) {
        if (padFromEvent(event)) {
            event.preventDefault();
        }
    });

    setInterval(flushQueue, 250);
})();
