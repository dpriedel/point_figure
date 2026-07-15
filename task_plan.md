# Task Plan: Eodhd WebSocket Streaming Failure Recovery

## Problem
Eodhd websocket client gets `{"status":500,"message":"Server error"}` during streaming. When this occurs, streaming stops permanently with no recovery.

## Root Cause
`on_read_subscribe()` in `src/Eodhd.cpp` returns early on subscription failure without:
- Starting the read loop (`StartReadLoop()`)
- Triggering reconnection (`start_reconnection()`)

Same issue exists in `on_write_subscribe()` error path.

## Solution
Add `start_reconnection()` + info log to three error paths in `src/Eodhd.cpp`:
1. `on_write_subscribe` — write failure (line 56-57)
2. `on_read_subscribe` — read error (line 71-72)
3. `on_read_subscribe` — non-success response (line 82-83)

## Additional Issues Found (2026-07-13 code review)
- Comma-space in subscribe/unsubscribe symbol strings (`", "` instead of `","`) — likely triggers server 500 errors
- DNS/TCP failures in `on_resolve`/`on_connect` don't trigger reconnection
- Ambiguous guard message doesn't indicate which limit was hit
- No way to recover after max retries reached (process must restart)

## Steps
- [x] Identify root cause in code review
- [x] Implement fix in Eodhd.cpp
- [x] Verify compilation (Debug + link)
- [x] Fix comma-space in subscribe/unsubscribe strings
- [x] Add reconnection to DNS/TCP failure paths
- [x] Split reconnection guard into specific messages
- [x] Add `ResetAndRestart()` public method for recovery after max retries
- [x] Fix shutdown hang: `start_reconnection()` exit paths set `had_signal_` before `ioc_.stop()`
- [ ] Test during market hours (Monday+)
