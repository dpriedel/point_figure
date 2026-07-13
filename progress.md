# Progress: Eodhd WebSocket Recovery Fix

## Completed
- [x] 2026-07-11 — Identified root cause: `on_read_subscribe()` and `on_write_subscribe()` return on error without triggering reconnection
- [x] 2026-07-11 — Added `start_reconnection()` + info log to three error paths in `src/Eodhd.cpp`
- [x] 2026-07-11 — Verified Debug build compiles and links successfully
- [x] 2026-07-13 — Fixed infinite retry loop: added separate `subscription_fail_count_` counter that accumulates across reconnect cycles
- [x] 2026-07-13 — Removed `reconnect_attempts_ = 0` reset from `on_handshake()` — successful handshake doesn't mean session is healthy
- [x] 2026-07-13 — Fixed misleading log message: "seconds" → "ms" for reconnect delay
- [x] 2026-07-13 — Backoff now uses `max(reconnect_attempts_, subscription_fail_count_)` so subscription failures accumulate delay
- [x] 2026-07-13 — Added guard: stops retrying after `max_subscription_fails_` (5) consecutive subscription failures
- [x] 2026-07-13 — Fixed comma-space in subscribe/unsubscribe symbol strings (`", "` → `","`)
- [x] 2026-07-13 — Added reconnection to `on_resolve` and `on_connect` failure paths
- [x] 2026-07-13 — Split reconnection guard into specific messages (disabled / max reconnect / max subscription)
- [x] 2026-07-13 — Added `ResetAndRestart()` public method for recovery after max retries

## Pending
- [ ] Test during market hours (Monday-Friday, 9:30 AM - 4:00 PM ET) to verify reconnection works after server error

## Files Changed
- `src/Streamer.h` — added `subscription_fail_count_`, `max_subscription_fails_`, and `ResetAndRestart()` declaration
- `src/Streamer.cpp` — updated constructors, removed handshake reset, fixed log unit, updated backoff logic, split guard messages, added DNS/TCP reconnection, implemented `ResetAndRestart()`
- `src/Eodhd.cpp` — three error paths increment `subscription_fail_count_`, success path resets it; fixed comma-space in subscribe/unsubscribe strings
