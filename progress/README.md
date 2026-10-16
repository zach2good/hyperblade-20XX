# Progress

`history.csv` gets a row each time `python tools/progress.py --record` runs after a score.
`status.svg` is the card at the top of the main README, drawn by `tools/status_card.py`.

Pull requests leave this folder and the README figures alone. Every six hours the `figures`
workflow scores main and commits all three when they have moved.

Two stretches of `history.csv` need care:

- Rows dated before 2026-09-01 count matched C runtime bytes above `0x004a0000` in
  `matched_bytes`, but not in `game_bytes`. They read up to about 23,000 bytes high, so don't
  compare them with later rows.
- Rows from 2,133 to 2,315 exact functions were written in afterwards from figures recorded at
  the time. `written_bytes` and `written_count` are blank there because nobody wrote them down.
  `game_bytes` moves in that stretch because the function inventory was corrected seven times.
