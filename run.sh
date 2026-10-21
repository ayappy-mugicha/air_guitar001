#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
# shortcut: Windows venv layout on Linux uses bin/activate
if [[ -f "$ROOT/air_guitar001/bin/activate" ]]; then
  # shellcheck source=/dev/null
  source "$ROOT/air_guitar001/bin/activate"
elif [[ -f "$ROOT/air_guitar001/Scripts/activate" ]]; then
  # shellcheck source=/dev/null
  source "$ROOT/air_guitar001/Scripts/activate"
fi
exec python "$ROOT/supervise.py"
