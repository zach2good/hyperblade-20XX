"""Compare v1.0.0.0's recorded parameter size against this project's signatures.

An FPO record's `cbParams` is the size of the parameters the callee finds on the stack.
Register-passed arguments are not in it, so `__thiscall` loses `this` and `__fastcall`
loses its first two. With that allowed for, the record gives the argument count, which reccmp
cannot see: an unused trailing `__cdecl` parameter leaves the callee's bytes unchanged.

It reads any pairing file with `v1000_address` and `v1001_address` columns, such as
`symbols/v1000_anchors.csv` or `symbols/v1000_propagated.csv`.

    python tools/fpo_arity.py                                  the literal anchors
    python tools/fpo_arity.py -p symbols/v1000_propagated.csv  the propagated set
    python tools/fpo_arity.py --source                         check the written source
                                                               rather than the inventory

The record describes v1.0.0.0, and a signature could have changed since, so a disagreement is
a place to look. Where the function has a direct call site, tools/check_call_arity.py settles
it from v1.0.0.1 itself.

Exit code 1 when anything disagrees.
"""

import argparse
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from check_call_arity import parameters, source_signatures, stack_dwords

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FPO = os.path.join(ROOT, "symbols", "v1000_fpo.csv")
ANCHORS = os.path.join(ROOT, "symbols", "v1000_anchors.csv")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")

# How many of a function's arguments its convention hands over in registers.
IN_REGISTERS = {"__thiscall": 1, "__fastcall": 2}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-p", "--pairs", default=ANCHORS, help="the pairing file to read")
    parser.add_argument("--source", action="store_true",
                        help="compare the written source instead of symbols/functions.csv")
    parser.add_argument("--all", action="store_true", help="list the agreements too")
    args = parser.parse_args()

    for path in (FPO, args.pairs, INVENTORY):
        if not os.path.isfile(path):
            sys.exit("fpo_arity: no such file: %s\n"
                     "Both v1000 files are derived from an executable this repository does "
                     "not ship. .gitignore names the commands that rebuild them." % path)

    recorded = {}
    with open(FPO, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            recorded[int(row["address"], 16)] = int(row["param_dwords"])

    rows = {}
    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            rows[int(row["address"], 16)] = row

    written = source_signatures() if args.source else {}

    checked = 0
    unrecorded = 0
    disagreements = []
    with open(args.pairs, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            source = int(row["v1000_address"], 16)
            target = int(row["v1001_address"], 16)
            if source not in recorded:
                unrecorded += 1
                continue

            if args.source:
                if target not in written:
                    continue
                declaration = written[target][2]
                head = declaration.split("(")[0]
                # A member function's `this` is implicit and never appears in the list,
                # so there is nothing to subtract. Only a free function spelled
                # __thiscall or __fastcall writes its register arguments out.
                convention = "__cdecl"
                if "::" not in head:
                    for candidate in IN_REGISTERS:
                        if candidate in head:
                            convention = candidate
                name = rows.get(target, {}).get("name", "0x%08X" % target)
            else:
                if target not in rows:
                    continue
                declaration = rows[target]["signature"]
                convention = rows[target]["convention"]
                name = rows[target]["name"]

            params = parameters(declaration)
            if params is None:
                continue
            total = stack_dwords(params)
            if total is None:
                continue
            in_registers = min(IN_REGISTERS.get(convention, 0), len(params))
            declared = total - stack_dwords(params[:in_registers])
            checked += 1
            if declared != recorded[source]:
                disagreements.append((target, name, convention, declared, recorded[source],
                                      source))
            elif args.all:
                print("agree     %-46s 0x%08X  %d" % (name, target, declared))

    for target, name, convention, declared, actual, source in sorted(disagreements):
        print("0x%08X  %-46s %-11s declared %d, v1.0.0.0 recorded %d  (v1000 0x%08X)"
              % (target, name, convention, declared, actual, source))

    subject = "written source" if args.source else "symbols/functions.csv"
    print("fpo_arity: %d pairs compared against %s, %d agree, %d disagree "
          "(%d pairs have no FPO record)"
          % (checked, subject, checked - len(disagreements), len(disagreements), unrecorded))
    return 1 if disagreements else 0


if __name__ == "__main__":
    sys.exit(main())
