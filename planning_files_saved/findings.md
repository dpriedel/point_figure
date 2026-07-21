# Findings: Eodhd WebSocket Subscription Failure

## Error Observed
```
[2026-07-08 09:31:53.485] [PF_Collect_logger] [error] Failed to get success code. Got: {"status":500,"message":"Server error"}
```

## Code Flow Analysis

### Normal path:
1. `on_handshake()` succeeds → sets `should_reconnect_ = true`, calls `OnConnected()`
2. `OnConnected()` sends subscribe message via async_write
3. `on_write_subscribe()` completes, starts async_read for response
4. `on_read_subscribe()` receives `{"status_code":200,...}` → calls `StartReadLoop()`
5. Read loop begins collecting streaming data

### Failure path (before fix):
1. Steps 1-3 same as normal
2. Server responds with `{"status":500,"message":"Server error"}`
3. `on_read_subscribe()` logs error and **returns** — no read loop, no reconnection
4. WebSocket sits idle; streaming data never collected again

### Why no automatic recovery?
After the early return, no `async_read` is active on the websocket. The 30s ping/pong timeout may or may not fire depending on whether Beast's timeout mechanism works without a pending operation. Either way, there's no logging to indicate recovery is being attempted, making it appear streaming died permanently.

## Response Format Note
The error `{"status":500,...}` uses `status` key (not `status_code`). This appears to be Eodhd's server-level error format, distinct from the normal subscription response format.

## Additional Findings (2026-07-13 code review)

### Comma-space in symbol strings (likely cause of 500 errors)
Subscribe/unsubscribe messages join symbols with `", "` (comma + space). EODHD docs require no spaces: `"symbols": "AAPL,TSLA"`. Server may reject `" MSFT"` (leading space) as unknown symbol, triggering 500.

### DNS/TCP failures are terminal
`on_resolve()` and `on_connect()` log error and return — no reconnection attempt. Transient network issues cause permanent streaming failure.

### No recovery after max retries
Once `subscription_fail_count_ >= 5`, IO context stops permanently. Only way to recover is restart process. Added `ResetAndRestart()` method to allow programmatic recovery.

### Shutdown hang after max retries (2026-07-15)
When `start_reconnection()` exhausts all retries, it calls `ioc_.stop()` but doesn't set `had_signal_ = true`. The timer thread (`WaitForTimer`) only exits when `had_signal_` is true OR market close arrives. Since neither condition was met, the main thread hung on `timer_task.get()` for hours until market close. Fixed by setting `*had_signal_ptr_ = true` before all three `ioc_.stop()` calls in `start_reconnection()`.
