"""Tell a survey tool whether a declaration's note says the change was already declined.

Headers word a declined change several ways, such as "Not retyped" or "Stays void*
deliberately". This shares one check across the surveys, so a note one tool honours is not
proposed again by another.

    from settled import isSettled
    if isSettled(commentTextAbove): continue

Limit: it reads only the note on the declaration itself. A note on the reader's class is not
found.
"""

MARKERS = (
    "not retyped",
    "deliberately",
    "neither is established",
    "measured and reverted",
    "left undone",
)


def isSettled(before):
    """True when the prose above a declaration says the trade was measured and declined."""
    low = before.lower()

    return any(m in low for m in MARKERS)
