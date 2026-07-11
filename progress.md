# Progress: Eodhd WebSocket Recovery Fix

## Completed
- [x] 2026-07-11 — Identified root cause: `on_read_subscribe()` and `on_write_subscribe()` return on error without triggering reconnection
- [x] 2026-07-11 — Added `start_reconnection()` + info log to three error paths in `src/Eodhd.cpp`
- [x] 2026-07-11 — Verified Debug build compiles and links successfully

## Pending
- [ ] Test during market hours (Monday-Friday, 9:30 AM - 4:00 PM ET) to verify reconnection works after server error

## Files Changed
- `src/Eodhd.cpp` — three locations in subscription handlers
