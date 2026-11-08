#!/usr/bin/env python3
"""Run a ProgrammaticToolset script file on an Unreal MCP port and print run()'s dict as JSON.
   uescript.py <port> <script.py>"""
import json, subprocess, sys, os
here = os.path.dirname(os.path.abspath(__file__))
port, path = sys.argv[1], sys.argv[2]
r = subprocess.run(["python3", os.path.join(here, "ue_mcp.py"), port, "call",
                    "editor_toolset.toolsets.programmatic.ProgrammaticToolset.execute_tool_script",
                    json.dumps({"script": open(path).read()})], capture_output=True, text=True)
out = r.stdout
try:
    v = json.loads(out)["returnValue"]
    v = json.loads(v) if isinstance(v, str) else v
    print(json.dumps(v, indent=1))
except Exception:
    print(out, r.stderr)
