"""Find sources newer than the linked image, or a report older than it.

A build that does not finish leaves the previous image and report in place, and every tool that
reads them answers with the previous tree's numbers. Shared with tools/prove_neutral.py.

Returns a list of complaints, empty when the report can be trusted.
"""
import os


def report_is_stale(root, image, report):
    if not os.path.exists(image):
        return ["no %s at all" % image]
    if not os.path.exists(report):
        return ["no %s at all" % report]

    linked = os.path.getmtime(image)
    late = []

    for base, unused_dirs, names in os.walk(os.path.join(root, "src")):
        for name in names:
            if not name.endswith((".c", ".cpp", ".h")):
                continue

            path = os.path.join(base, name)

            if os.path.getmtime(path) > linked:
                late.append("%s is newer than the image" % os.path.relpath(path, root))

    if os.path.getmtime(report) < linked:
        late.append("%s is older than the image" % os.path.relpath(report, root))

    return late
