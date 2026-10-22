(function () {
    var lastFocus = null;

    function dialog() {
        return document.getElementById("guide-dialog");
    }

    function openButton() {
        return document.getElementById("guide-open");
    }

    function sheet() {
        var root = dialog();
        return root ? root.querySelector(".guide-sheet") : null;
    }

    function isOpen() {
        var root = dialog();
        return Boolean(root && !root.hasAttribute("hidden"));
    }

    function focusables() {
        var panel = sheet();
        if (!panel) {
            return [];
        }
        return Array.prototype.slice.call(
            panel.querySelectorAll(
                'button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])'
            )
        ).filter(function (node) {
            return !node.disabled && node.getAttribute("aria-hidden") !== "true";
        });
    }

    function setOpen(nextOpen) {
        var root = dialog();
        var toggle = openButton();
        if (!root) {
            return;
        }
        if (nextOpen) {
            lastFocus = document.activeElement;
            root.removeAttribute("hidden");
            document.documentElement.classList.add("guide-open");
            if (toggle) {
                toggle.setAttribute("aria-expanded", "true");
            }
            var closeBtn = document.getElementById("guide-close");
            window.requestAnimationFrame(function () {
                if (closeBtn) {
                    closeBtn.focus();
                }
            });
            return;
        }
        root.setAttribute("hidden", "");
        document.documentElement.classList.remove("guide-open");
        if (toggle) {
            toggle.setAttribute("aria-expanded", "false");
        }
        if (lastFocus && typeof lastFocus.focus === "function") {
            lastFocus.focus();
        } else if (toggle) {
            toggle.focus();
        }
    }

    window.airGuitarGuideIsOpen = isOpen;

    document.addEventListener("click", function (event) {
        var target = event.target;
        if (!target || !target.closest) {
            return;
        }
        if (target.closest("#guide-open")) {
            event.preventDefault();
            setOpen(true);
            return;
        }
        if (target.closest("#guide-close")) {
            event.preventDefault();
            setOpen(false);
            return;
        }
        if (isOpen() && target.classList && target.classList.contains("guide-backdrop")) {
            setOpen(false);
        }
    });

    document.addEventListener("keydown", function (event) {
        if (!isOpen()) {
            return;
        }
        if (event.key === "Escape") {
            event.preventDefault();
            setOpen(false);
            return;
        }
        if (event.key !== "Tab") {
            return;
        }
        var nodes = focusables();
        if (!nodes.length) {
            return;
        }
        var first = nodes[0];
        var last = nodes[nodes.length - 1];
        if (event.shiftKey && document.activeElement === first) {
            event.preventDefault();
            last.focus();
        } else if (!event.shiftKey && document.activeElement === last) {
            event.preventDefault();
            first.focus();
        }
    });
})();
