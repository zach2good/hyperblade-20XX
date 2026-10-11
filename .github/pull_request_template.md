<!-- What the change does and why, in a line or two. -->

### A good change

- [ ] It's small: one function, or one clean-up of one shape, per commit.
- [ ] `python tools/check_regressions.py` reports no drop, or the commit states what the drop cost.
- [ ] `python tools/checkers.py` passes, and `clang-format` 22 leaves the source unchanged.
- [ ] It contains no `HYPERX.EXE`, Ghidra project, disassembly listing, or reccmp HTML or JSON report.
- [ ] It keeps the program's behaviour. It doesn't fix the original's bugs or move past C++98.

CONTRIBUTING.md gives the full rules.
