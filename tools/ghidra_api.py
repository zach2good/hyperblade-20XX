"""Thin wrapper over the GhidraMCP HTTP bridge.

The bridge serves from a running Ghidra GUI and most endpoints return plain text rather than
JSON. The GHIDRA_BRIDGE environment variable names it, and defaults to http://127.0.0.1:8089.
Calls share one session and a thread pool, so a whole-program sweep is practical.

Read only. Nothing in this file writes to the database.
"""

import json
import os
import sys
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor

BASE = os.environ.get("GHIDRA_BRIDGE", "http://127.0.0.1:8089").rstrip("/")


class GhidraDown(Exception):
    pass


def get(endpoint, **params):
    url = BASE + "/" + endpoint.lstrip("/")
    if params:
        url += "?" + urllib.parse.urlencode(params)
    try:
        with urllib.request.urlopen(url, timeout=60) as response:
            return response.read().decode("utf-8", "replace")
    except Exception as error:
        raise GhidraDown("%s: %s" % (endpoint, error))


def get_json(endpoint, **params):
    text = get(endpoint, **params)
    try:
        return json.loads(text)
    except ValueError:
        return text


def check():
    """Return the loaded program name, or raise."""
    reply = get("check_connection")
    if "HYPERX" not in reply:
        raise GhidraDown("bridge is up but HYPERX.EXE is not loaded: %s" % reply.strip())
    return reply.strip()


def parallel(endpoint, addresses, workers=10, **params):
    """Call one endpoint for many addresses. Yields (address, text) in input order."""
    def one(address):
        try:
            return address, get(endpoint, address=address, **params)
        except GhidraDown as error:
            return address, "ERROR %s" % error

    with ThreadPoolExecutor(max_workers=workers) as pool:
        yield from pool.map(one, addresses)


if __name__ == "__main__":
    try:
        print(check())
    except GhidraDown as error:
        print("Ghidra bridge not available: %s" % error)
        print("Open Ghidra, load HYPERX.EXE, and enable File > Configure > Miscellaneous > GhidraMCP.")
        sys.exit(1)
