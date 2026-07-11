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

## Steps
- [x] Identify root cause in code review
- [x] Implement fix in Eodhd.cpp
- [x] Verify compilation (Debug + link)
- [ ] Test during market hours (Monday+)
