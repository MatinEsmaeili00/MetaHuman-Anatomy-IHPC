"""File-based Python bridge for the Unreal Editor (no network).

Run once inside the editor (Output Log > Cmd):
    py "/Users/mesmae6/GitHub/MetaHuman-Anatomy-IHPC/tools/ue/claude_bridge.py"

It watches <Project>/Saved/ClaudeBridge/inbox for *.py files, runs each one on the game thread,
writes stdout + errors to <Project>/Saved/ClaudeBridge/outbox/<name>.json, and moves the script to done/.
Only files on this Mac's disk can trigger it. Nothing listens on the network.

Stop it:   py "/Users/mesmae6/GitHub/MetaHuman-Anatomy-IHPC/tools/ue/claude_bridge.py" stop
(It also stops when the editor closes.)
"""
import contextlib
import io
import json
import os
import sys
import time
import traceback

import unreal

ROOT = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ClaudeBridge")
INBOX, OUTBOX, DONE = (os.path.join(ROOT, d) for d in ("inbox", "outbox", "done"))
for d in (INBOX, OUTBOX, DONE):
    os.makedirs(d, exist_ok=True)

_STATE_KEY = "_claude_bridge_handle"


RUNNING = os.path.join(ROOT, "running")
os.makedirs(RUNNING, exist_ok=True)
_busy = [False]


def _run_file(path):
    # claim the job first so a nested Slate tick (e.g. during a modal save dialog) can't run it twice
    name = os.path.basename(path)
    claimed = os.path.join(RUNNING, name)
    os.replace(path, claimed)
    path = claimed
    buf = io.StringIO()
    ok, err = True, None
    t0 = time.time()
    with open(path, "r", encoding="utf-8") as f:
        code = f.read()
    g = {"__name__": "__main__", "__file__": path, "unreal": unreal}
    with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(buf):
        try:
            exec(compile(code, path, "exec"), g)
        except Exception:
            ok, err = False, traceback.format_exc()
    result = {"name": name, "ok": ok, "error": err, "output": buf.getvalue(), "seconds": round(time.time() - t0, 3)}
    tmp = os.path.join(OUTBOX, name + ".json.tmp")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(result, f)
    os.replace(tmp, os.path.join(OUTBOX, name + ".json"))
    os.replace(path, os.path.join(DONE, name))
    unreal.log(f"[ClaudeBridge] ran {name} ok={ok} in {result['seconds']}s")


_last = [0.0]


def _tick(delta):
    now = time.time()
    if _busy[0] or now - _last[0] < 0.25:
        return
    _last[0] = now
    try:
        files = sorted(f for f in os.listdir(INBOX) if f.endswith(".py"))
    except FileNotFoundError:
        return
    _busy[0] = True
    try:
        for f in files:
            _run_file(os.path.join(INBOX, f))
    finally:
        _busy[0] = False


def start():
    stop()
    handle = unreal.register_slate_post_tick_callback(_tick)
    setattr(sys, _STATE_KEY, handle)
    unreal.log(f"[ClaudeBridge] watching {INBOX}")


def stop():
    handle = getattr(sys, _STATE_KEY, None)
    if handle is not None:
        unreal.unregister_slate_post_tick_callback(handle)
        setattr(sys, _STATE_KEY, None)
        unreal.log("[ClaudeBridge] stopped")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "stop":
        stop()
    else:
        start()
