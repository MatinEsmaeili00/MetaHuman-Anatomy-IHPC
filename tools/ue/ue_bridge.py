#!/usr/bin/env python3
"""Client for tools/claude_bridge.py: drop a script into the editor's inbox and print its result.

    ue_bridge.py path/to/script.py
    ue_bridge.py -c "import unreal; print(unreal.SystemLibrary.get_engine_version())"
"""
import argparse
import json
import os
import sys
import time
import uuid

PROJECT = os.environ.get("UE_PROJECT", "/Users/mesmae6/Perforce/maclab_digitalhuman_main/Claude/MetaHumanTestCOPY2")
ROOT = os.path.join(PROJECT, "Saved", "ClaudeBridge")


def run(code: str, label: str = "job", timeout: float = 600.0) -> dict:
    inbox, outbox = os.path.join(ROOT, "inbox"), os.path.join(ROOT, "outbox")
    if not os.path.isdir(inbox):
        raise SystemExit(f"Bridge folder missing: {inbox}. Run claude_bridge.py inside the editor first.")
    name = f"{time.strftime('%H%M%S')}_{label}_{uuid.uuid4().hex[:6]}.py"
    tmp = os.path.join(inbox, name + ".tmp")
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(code)
    os.replace(tmp, os.path.join(inbox, name))
    out = os.path.join(outbox, name + ".json")
    deadline = time.time() + timeout
    while time.time() < deadline:
        if os.path.exists(out):
            with open(out, encoding="utf-8") as f:
                return json.load(f)
        time.sleep(0.2)
    raise SystemExit(f"Timed out waiting for {name} (is the bridge running in the editor?)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("script", nargs="?")
    ap.add_argument("-c", dest="code")
    ap.add_argument("--timeout", type=float, default=600.0)
    a = ap.parse_args()
    if a.code:
        res = run(a.code, "inline", a.timeout)
    elif a.script:
        with open(a.script, encoding="utf-8") as f:
            res = run(f.read(), os.path.splitext(os.path.basename(a.script))[0][:24], a.timeout)
    else:
        ap.error("script path or -c CODE required")
    sys.stdout.write(res.get("output", ""))
    if not res.get("ok"):
        sys.stdout.write("\n--- ERROR ---\n" + (res.get("error") or ""))
        sys.exit(1)


if __name__ == "__main__":
    main()
