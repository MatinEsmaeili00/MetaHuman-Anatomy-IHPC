#!/usr/bin/env python3
"""Tiny Unreal MCP (streamable HTTP) client.

Usage:
  ue_mcp.py <port> list                       -> list toolsets
  ue_mcp.py <port> describe <toolset>         -> describe one toolset
  ue_mcp.py <port> call <tool> '<json args>'  -> call a toolset tool via call_tool
  ue_mcp.py <port> raw <mcp-tool> '<json>'    -> call a native MCP tool directly
"""
import json
import sys
import urllib.request

port = sys.argv[1]
URL = f"http://127.0.0.1:{port}/mcp"
HDR = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}


def post(payload, sid=None, timeout=600):
    h = dict(HDR)
    if sid:
        h["Mcp-Session-Id"] = sid
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=h, method="POST")
    with urllib.request.urlopen(req, timeout=timeout) as r:
        body = r.read().decode("utf-8", "replace")
        return r.headers.get("Mcp-Session-Id"), (json.loads(body) if body.strip() else None)


sid, _ = post({"jsonrpc": "2.0", "id": 1, "method": "initialize",
               "params": {"protocolVersion": "2025-06-18", "capabilities": {},
                          "clientInfo": {"name": "claude-cli", "version": "0"}}})
post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)


def tool(name, args):
    _, r = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call",
                 "params": {"name": name, "arguments": args}}, sid)
    if r is None:
        return ""
    if "error" in r:
        return "ERROR: " + json.dumps(r["error"])
    res = r.get("result", {})
    out = []
    for i, c in enumerate(res.get("content", [])):
        if c.get("type") == "image" and c.get("data"):
            # Save images (viewport / editor captures) instead of printing base64.
            import base64, os
            path = os.environ.get("UE_MCP_IMAGE", "/tmp/ue_mcp_capture.png")
            if i:
                root, ext = os.path.splitext(path)
                path = f"{root}_{i}{ext}"
            with open(path, "wb") as f:
                f.write(base64.b64decode(c["data"]))
            out.append(f"[image saved: {path}]")
        else:
            text = c.get("text", json.dumps(c)[:2000])
            try:  # toolset captures come back as {"returnValue": {"mimeType": "image/png", "data": "<base64>"}}
                rv = json.loads(text).get("returnValue")
                if isinstance(rv, dict) and str(rv.get("mimeType", "")).startswith("image/") and rv.get("data"):
                    import base64, os
                    path = os.environ.get("UE_MCP_IMAGE", "/tmp/ue_mcp_capture.png")
                    with open(path, "wb") as f:
                        f.write(base64.b64decode(rv["data"]))
                    text = f"[image saved: {path}]"
            except Exception:
                pass
            out.append(text)
    if res.get("isError"):
        out.insert(0, "TOOL ERROR:")
    return "\n".join(out)


cmd = sys.argv[2]
if cmd == "list":
    print(tool("list_toolsets", {}))
elif cmd == "describe":
    print(tool("describe_toolset", {"toolset_name": sys.argv[3]}))
elif cmd == "call":
    args = json.loads(sys.argv[4]) if len(sys.argv) > 4 else {}
    ts, _, tn = sys.argv[3].rpartition(".")
    print(tool("call_tool", {"toolset_name": ts, "tool_name": tn, "arguments": args}))
elif cmd == "raw":
    args = json.loads(sys.argv[4]) if len(sys.argv) > 4 else {}
    print(tool(sys.argv[3], args))
