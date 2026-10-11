"""Verify a dispatch table's content against the original, slot by slot.

reccmp scores bytes. A table filler can score well and still leave a slot unwritten or pointing
at the wrong handler, which crashes at run time.

It reads the original's instruction listing from the Ghidra bridge, resolves every
`MOV [absolute], <code address>` store (including ones VC6 hoists into a register), names each
target from the `// FUNCTION: HYPERX` markers in src/, and compares that map with the
assignments the source file makes into the given arrays.

    python tools/table_content.py 0x0046ac10 src/planvm/planvm.cpp \
        g_pfnPlanOpcodeHandlers=0x004cf084 g_pfnPlanActionHandlers=0x004cf2b8

Read only. It never writes to the Ghidra database.
"""
import glob
import os
import re
import sys
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
from ghidra_api import BASE  # noqa: E402

WRITES_DESTINATION = {
    "MOV", "MOVZX", "MOVSX", "LEA", "POP", "XCHG",
    "ADD", "ADC", "SUB", "SBB", "AND", "OR", "XOR", "NEG", "NOT", "INC", "DEC",
    "IMUL", "SHL", "SAL", "SHR", "SAR", "ROL", "ROR", "RCL", "RCR", "SHLD", "SHRD",
}


def marker_map():
    """Every `// FUNCTION: HYPERX 0x...` marker in src/, as address -> names."""
    out = {}
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True) + glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        with open(path, encoding="utf-8", errors="replace") as handle:
            lines = handle.read().split("\n")
        for index, line in enumerate(lines):
            found = re.search(r"//\s*FUNCTION:\s*HYPERX\s+(0x[0-9a-fA-F]+)", line)
            if not found:
                continue
            for ahead in range(index + 1, min(index + 6, len(lines))):
                name = re.search(r"([A-Za-z_][A-Za-z0-9_:~]*)\s*\(", lines[ahead])
                if name:
                    out.setdefault(int(found.group(1), 16), set()).add(name.group(1))
                    break
    return out


def original_stores(address):
    """Absolute-address stores in one original function, as address -> value.

    VC6 hoists an immediate repeated across two slots into a register, so the
    register's last immediate load is tracked and resolved at the store.
    """
    url = "%s/disassemble_function?address=%s" % (BASE, address)
    with urllib.request.urlopen(url, timeout=90) as reply:
        listing = reply.read().decode("utf-8", "replace")
    registers = {}
    stores = {}
    for line in listing.split("\n"):
        line = line.strip()
        load = re.match(r"[0-9a-f]+: MOV (E[A-Z]{2}),0x([0-9a-f]+)\s*$", line)
        if load:
            registers[load.group(1)] = int(load.group(2), 16)
            continue
        # An instruction that writes its destination register ends the tracked value.
        # CMP, TEST and PUSH only read theirs.
        clobber = re.match(r"[0-9a-f]+: ([A-Z]+) (E[A-Z]{2})\b", line)
        if clobber and clobber.group(1) in WRITES_DESTINATION:
            registers.pop(clobber.group(2), None)
            continue
        if re.match(r"[0-9a-f]+: CALL ", line):
            for scratch in ("EAX", "ECX", "EDX"):
                registers.pop(scratch, None)
            continue
        store = re.match(
            r"[0-9a-f]+: MOV (?:dword ptr )?\[0x([0-9a-f]+)\],(0x[0-9a-f]+|E[A-Z]{2})\s*$",
            line,
        )
        if not store:
            continue
        operand = store.group(2)
        if operand in registers:
            value = registers[operand]
        elif operand.startswith("0x"):
            value = int(operand, 16)
        else:
            continue
        stores[int(store.group(1), 16)] = value
    return stores


def source_stores(path, arrays):
    """`array[index] = Name;` in one source file, as absolute address -> name."""
    with open(os.path.join(ROOT, path), encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    out = {}
    for name, base in arrays.items():
        # A slot may be spelled bare, behind a cast, or behind a one-argument macro that casts
        # for it. The capture takes the whole qualified name, such as PlanCond::SetAnonLocal.
        pattern = (re.escape(name) + r"\[(\d+)\]\s*=\s*"
                   r"(?:\([\w \*:]+\)\s*)*"
                   r"(?:[A-Za-z_][A-Za-z0-9_]*\s*\(\s*)?"
                   r"(?:\([\w \*:]+\)\s*)*"
                   r"([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*)")
        for found in re.finditer(pattern, text):
            value = found.group(2)

            # `slot = (T*)new Thing;` fills an object table at run time, not a dispatch table,
            # so the array is skipped.
            if value == "new":
                continue

            out[base + int(found.group(1)) * 4] = value
    return out


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2
    address = sys.argv[1]
    path = sys.argv[2]
    arrays = {}
    for pair in sys.argv[3:]:
        name, base = pair.split("=")
        arrays[name] = int(base, 16)

    names = marker_map()
    original = original_stores(address)
    ours = source_stores(path, arrays)

    faults = 0
    for slot in sorted(set(original) | set(ours)):
        value = original.get(slot)
        named = ours.get(slot)
        known = names.get(value, set()) if value is not None else set()
        if value is None:
            print("SOURCE ONLY   %s = %s" % (hex(slot), named))
            faults += 1
        elif named is None:
            print("UNWRITTEN     %s should be %s (%s)"
                  % (hex(slot), hex(value), ", ".join(sorted(known)) or "unnamed"))
            faults += 1
        elif named not in known:
            print("WRONG HANDLER %s: original %s (%s), source %s"
                  % (hex(slot), hex(value), ", ".join(sorted(known)) or "unnamed", named))
            faults += 1

    print("%d slots in the original, %d in the source, %d faults"
          % (len(original), len(ours), faults))
    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
