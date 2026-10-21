"""Supervise pose.py and guitar_dash.py in separate consoles."""

from __future__ import annotations

import json
import os
import select
import signal
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
SCRIPTS = ("pose.py", "guitar_dash.py")
MAX_CONSECUTIVE_FAILURES = 3
STABLE_UPTIME_S = 30.0
GRACEFUL_TIMEOUT_S = 5.0
POLL_S = 0.5

CREATE_NEW_CONSOLE = 0x00000010
CREATE_NEW_PROCESS_GROUP = 0x00000200

_stop_requested = False


def on_child_exit(consecutive_failures: int, uptime_s: float) -> tuple[bool, int]:
    """Return (should_restart_pair, new_consecutive_failures)."""
    if uptime_s < STABLE_UPTIME_S:
        new_failures = consecutive_failures + 1
    else:
        new_failures = consecutive_failures
    should_restart = new_failures < MAX_CONSECUTIVE_FAILURES
    return should_restart, new_failures


def stable_uptime_reached(uptime_s: float) -> bool:
    return uptime_s >= STABLE_UPTIME_S


@dataclass
class ManagedChild:
    script: str
    proc: subprocess.Popen | None = None
    uses_terminal_wrapper: bool = False


def _supervised_env() -> dict[str, str]:
    env = os.environ.copy()
    env["AIR_GUITAR_SUPERVISED"] = "1"
    return env


def _script_in_cmdline(cmdline: str, script: str) -> bool:
    return script in cmdline.replace("\\", "/")


def _iter_python_processes() -> list[tuple[int, str]]:
    my_pid = os.getpid()
    rows: list[tuple[int, str]] = []
    if sys.platform == "win32":
        try:
            raw = subprocess.check_output(
                [
                    "powershell",
                    "-NoProfile",
                    "-Command",
                    "Get-CimInstance Win32_Process -Filter \"Name='python.exe'\" "
                    "| Select-Object ProcessId,CommandLine "
                    "| ConvertTo-Json -Compress",
                ],
                text=True,
                stderr=subprocess.DEVNULL,
            ).strip()
        except (subprocess.CalledProcessError, OSError):
            return rows
        if not raw:
            return rows
        data = json.loads(raw)
        if isinstance(data, dict):
            data = [data]
        for item in data:
            pid = int(item.get("ProcessId", 0))
            cmd = item.get("CommandLine") or ""
            if pid and pid != my_pid:
                rows.append((pid, cmd))
    else:
        proc_root = Path("/proc")
        if not proc_root.is_dir():
            return rows
        for entry in proc_root.iterdir():
            if not entry.name.isdigit():
                continue
            pid = int(entry.name)
            if pid == my_pid:
                continue
            cmdline_path = entry / "cmdline"
            try:
                raw = cmdline_path.read_bytes().replace(b"\x00", b" ").decode(
                    "utf-8", errors="replace"
                )
            except OSError:
                continue
            if "python" in raw.lower():
                rows.append((pid, raw))
    return rows


def _is_target_leftover(cmdline: str) -> bool:
    return any(_script_in_cmdline(cmdline, s) for s in SCRIPTS)


def kill_leftovers() -> None:
    for pid, cmdline in _iter_python_processes():
        if not _is_target_leftover(cmdline):
            continue
        _terminate_pid(pid)


def _terminate_pid(pid: int) -> None:
    if sys.platform == "win32":
        subprocess.run(
            ["taskkill", "/PID", str(pid), "/T"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    else:
        try:
            os.kill(pid, signal.SIGTERM)
            time.sleep(GRACEFUL_TIMEOUT_S)
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass


def _find_terminal_launcher() -> str | None:
    import shutil

    for name in (
        "gnome-terminal",
        "x-terminal-emulator",
        "lxterminal",
        "xfce4-terminal",
        "konsole",
    ):
        if shutil.which(name):
            return name
    return None


def _unix_terminal_argv(terminal: str, script: str) -> list[str]:
    inner = (
        f'cd "{PROJECT_ROOT}" && '
        f'export AIR_GUITAR_SUPERVISED=1 && '
        f'"{sys.executable}" "{PROJECT_ROOT / script}"; '
        f'echo; read -p "終了しました。Enterで閉じます..." _'
    )
    if terminal == "gnome-terminal":
        return [terminal, "--", "bash", "-lc", inner]
    if terminal in ("x-terminal-emulator", "lxterminal", "xfce4-terminal"):
        return [terminal, "-e", f"bash -lc {json.dumps(inner)}"]
    if terminal == "konsole":
        return [terminal, "-e", "bash", "-lc", inner]
    return [terminal, "-e", "bash", "-lc", inner]


def start_child(script: str) -> ManagedChild:
    env = _supervised_env()
    script_path = PROJECT_ROOT / script
    if sys.platform == "win32":
        proc = subprocess.Popen(
            [sys.executable, str(script_path)],
            cwd=PROJECT_ROOT,
            env=env,
            creationflags=CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
        )
        return ManagedChild(script=script, proc=proc, uses_terminal_wrapper=False)

    terminal = _find_terminal_launcher()
    if terminal:
        argv = _unix_terminal_argv(terminal, script)
        proc = subprocess.Popen(argv, cwd=PROJECT_ROOT, env=env)
        return ManagedChild(script=script, proc=proc, uses_terminal_wrapper=True)

    proc = subprocess.Popen(
        [sys.executable, str(script_path)],
        cwd=PROJECT_ROOT,
        env=env,
    )
    return ManagedChild(script=script, proc=proc, uses_terminal_wrapper=False)


def _graceful_stop(child: ManagedChild) -> None:
    proc = child.proc
    if proc is None or proc.poll() is not None:
        return
    if child.uses_terminal_wrapper:
        proc.terminate()
    elif sys.platform == "win32":
        try:
            proc.send_signal(signal.CTRL_BREAK_EVENT)
        except OSError:
            proc.terminate()
    else:
        proc.send_signal(signal.SIGINT)
    try:
        proc.wait(timeout=GRACEFUL_TIMEOUT_S)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def stop_children(children: list[ManagedChild]) -> None:
    for child in children:
        _graceful_stop(child)


def _stdin_key_pressed() -> bool:
    if sys.platform == "win32":
        import msvcrt

        return msvcrt.kbhit()
    return bool(select.select([sys.stdin], [], [], 0)[0])


def _consume_stdin_key() -> None:
    if sys.platform == "win32":
        import msvcrt

        while msvcrt.kbhit():
            msvcrt.getch()
    else:
        if select.select([sys.stdin], [], [], 0)[0]:
            sys.stdin.read(1)


def _on_sigint(_signum: int, _frame: object) -> None:
    global _stop_requested
    _stop_requested = True


def _pair_alive(children: list[ManagedChild]) -> bool:
    return all(c.proc is not None and c.proc.poll() is None for c in children)


def _any_child_dead(children: list[ManagedChild]) -> bool:
    return any(c.proc is not None and c.proc.poll() is not None for c in children)


def _start_pair() -> tuple[list[ManagedChild], float]:
    children = [start_child(s) for s in SCRIPTS]
    return children, time.monotonic()


def run_supervisor() -> int:
    global _stop_requested
    signal.signal(signal.SIGINT, _on_sigint)
    if sys.platform == "win32" and hasattr(signal, "SIGBREAK"):
        signal.signal(signal.SIGBREAK, _on_sigint)

    print("残骸プロセスを確認しています...")
    kill_leftovers()

    consecutive_failures = 0
    children, pair_started = _start_pair()
    stable_reset_done = False

    print("pose.py と guitar_dash.py を別ウィンドウで起動しました。")
    print("このウィンドウで Ctrl+C または何かキーを押すと、すべて終了します。")

    try:
        while not _stop_requested:
            if _stdin_key_pressed():
                _consume_stdin_key()
                print("キー入力を受け付けました。終了します...")
                break

            uptime = time.monotonic() - pair_started
            if not stable_reset_done and stable_uptime_reached(uptime) and _pair_alive(children):
                consecutive_failures = 0
                stable_reset_done = True

            if _any_child_dead(children):
                dead = [c.script for c in children if c.proc and c.proc.poll() is not None]
                print(f"子プロセスが終了しました: {', '.join(dead)}")
                stop_children(children)
                should_restart, consecutive_failures = on_child_exit(
                    consecutive_failures, uptime
                )
                if not should_restart:
                    print(
                        f"連続 {MAX_CONSECUTIVE_FAILURES} 回の再起動に失敗しました。"
                        "監督プロセスを終了します。"
                    )
                    return 1
                print("ペアを再起動します...")
                children, pair_started = _start_pair()
                stable_reset_done = False
                continue

            time.sleep(POLL_S)
    finally:
        stop_children(children)
        kill_leftovers()

    print("すべてのプロセスを終了しました。")
    return 0


def main() -> None:
    raise SystemExit(run_supervisor())


if __name__ == "__main__":
    main()
