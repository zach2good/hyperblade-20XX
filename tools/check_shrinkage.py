"""Report tracked files that lost most of their content since HEAD.

A generator that appends with

    open(path, 'w').write(open(path).read() + extra)

truncates the file, because the open for writing runs before the read. The build then fails
somewhere unrelated. Run this before committing.

    python tools/check_shrinkage.py
"""
import subprocess
import sys

LIMIT = 0.5

diff = subprocess.check_output(
    ['git', 'diff', '--numstat', 'HEAD', '--', 'src', 'tools', 'docs']).decode('utf-8')

faults = 0

for line in diff.splitlines():
    parts = line.split()
    if len(parts) != 3 or not parts[0].isdigit() or not parts[1].isdigit():
        continue
    added, removed, path = int(parts[0]), int(parts[1]), parts[2]
    try:
        before = len(subprocess.check_output(
            ['git', 'show', 'HEAD:' + path]).decode('utf-8', 'replace').splitlines())
    except subprocess.CalledProcessError:
        continue
    if before and removed - added > before * LIMIT:
        faults += 1
        print('%s lost %d of %d lines' % (path, removed - added, before))

print('%d files shrank by more than half' % faults)
sys.exit(1 if faults else 0)
