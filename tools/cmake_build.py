"""Run `cmake --build` and run it again when NMake stops with U1054.

NMake sometimes fails to write its temporary inline file and stops with U1054. The fault is
transient, and the same build succeeds when run again. Any other failure ends the run.

    python tools/cmake_build.py build
    python tools/cmake_build.py build --target verify

The arguments go to `cmake --build` unchanged. Exit code is the last build's.
"""

import subprocess
import sys

INLINE_FILE_FAULT = "U1054"
ATTEMPTS = 3


def main():
    status = 1
    for attempt in range(1, ATTEMPTS + 1):
        result = subprocess.run(["cmake", "--build"] + sys.argv[1:], capture_output=True, text=True,
                                errors="replace")
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        sys.stdout.flush()
        status = result.returncode
        if status == 0 or INLINE_FILE_FAULT not in result.stdout + result.stderr:
            break
        print("NMake stopped with %s on attempt %d of %d" % (INLINE_FILE_FAULT, attempt, ATTEMPTS),
              flush=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
