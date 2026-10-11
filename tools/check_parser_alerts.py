# Report files whose annotations reccmp's own parser rejects.
#
# The parser abandons a file when it cannot make sense of a marker, and every FUNCTION in it
# silently drops out of the report. tools/check_markers_resolve.py sees the hole but not why.
#
# A common cause is a STRING marker whose literal is not on the next line (NO_SUITABLE_NAME).
#
#   python tools/check_parser_alerts.py
#
# Exit code 1 when any alert is raised.

import glob
import sys

from reccmp.parser.parser import DecompParser

alerts = []

for path in sorted(glob.glob("src/**/*.cpp", recursive=True)) + sorted(glob.glob("src/**/*.h", recursive=True)):
    parser = DecompParser()

    for line in open(path, encoding="utf-8", errors="replace"):
        parser.read_line(line)

    for alert in parser.alerts:
        code = getattr(alert.code, "name", alert.code)
        alerts.append((path, code, alert.line_number))

for path, code, line in alerts:
    print("%s:%d  %s" % (path, line, code))

print("%d files scanned, %d parser alerts" % (
    len(glob.glob("src/**/*.cpp", recursive=True)) + len(glob.glob("src/**/*.h", recursive=True)), len(alerts)))

sys.exit(1 if alerts else 0)
