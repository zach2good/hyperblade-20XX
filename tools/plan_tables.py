"""The plan handler registrations in src/planvm/planvm.cpp, with their indices resolved.

An index is a literal or a sum of names from src/planvm/planfunctionids.h.
"""

import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLE_FILE = os.path.join(ROOT, "src", "planvm", "planvm.cpp")
IDS_FILE = os.path.join(ROOT, "src", "planvm", "planfunctionids.h")
REGISTER = re.compile(r"(g_pfnPlan\w+Handlers)\[([^\]]+)\](?:\.\w+)?\s*=\s*(?:\(\w+\*?\)\s*)?([\w:]+);")


def registrations():
    """Each (table, index, body) in source order."""
    ids = open(IDS_FILE, encoding="utf-8").read()
    values = dict((name, int(value, 0)) for name, value in re.findall(r"#define (\w+)[ \t]+(\w+)", ids))
    text = open(TABLE_FILE, encoding="utf-8").read()
    out = []
    for table, index, body in REGISTER.findall(text):
        terms = [t.strip() for t in index.split("+")]
        out.append((table, sum(values[t] if t in values else int(t, 0) for t in terms), body))
    return out
