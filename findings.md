# Findings: PF_CollectDataApp Codebase Analysis

## Architecture Assessment

### Current State — Monolithic God Class
- `PF_CollectDataApp`: ~2400 lines, ~52 member variables
- Single dispatcher in `Run()` routes to 6 mode-specific methods based on CLI args
- All modes compiled into one binary with all dependencies linked

### Mode Dispatch Structure
```
Run()
├── new_data_source_ == streaming ──► Run_Streaming()
├── mode_ == daily_scan             ──► Run_DailyScan()
├── new_data_source_ == file
│   ├── mode_ == load    ──► Run_Load()
│   └── mode_ == update  ──► Run_Update()
└── new_data_source_ == DB
    ├── mode_ == load    ──► Run_LoadFromDB()
    └── mode_ == update  ──► Run_UpdateFromDB()
```

### Existing Separation (Good)
- Domain model (`PF_Chart`, `PF_Column`, `Boxes`, `PF_Signals`) isolated in `libPF_Chart.a`
- Data source abstraction via `RemoteDataSource` base class (Tiingo/Eodhd interchangeable)
- Database access encapsulated in `PF_DB` class
- CLI parsing uses CLI11 framework cleanly

### Tight Coupling (Needs Work)
- App class handles: argument validation, logging, all 6 run modes, streaming pipeline, ATR computation, file/DB persistence, signal handling, statistics — all inline
- ~50 member variables mixing concerns from unrelated modes
- `CheckArgs()` is a massive function with mode-specific conditional blocks
- No service layer — app directly instantiates `PF_DB`, `Eodhd`/`Tiingo`, calls methods inline

### ChartDirector Dependency
- Closed-source third-party library (`-lchartdir`)
- Used in `ConstructChartGraphic.cpp` (per-binary) and `PF_Chart_CD.cpp` (in `libPF_Chart.a`)
- NOT needed by scanner — pure DB statistics, no graphics generation
- Needed by loader, updater, streamer for chart graphic generation
- `ConstructChartGraphic.cpp` should stay as shared source (not in library) to give per-binary control over this closed-source dependency

### Test Infrastructure
- E2e tests: 28 tests across 7 fixtures in `../PF_Test/EndToEnd_Test.cpp`
- Tests construct `PF_CollectDataApp(const vector<string>& tokens)` directly
- Unit tests: domain logic in `../PF_Test/Unit_Test.cpp` (tests PF_Chart, Boxes, etc.)
- E2e makefile compiles test + app sources together, links libPF_Chart

### Test Distribution by Mode
| Fixture | Tests | Maps To |
|---|---|---|
| `SingleFileEndToEnd` | 2 | pf_loader |
| `LoadAndUpdate` | 1 | pf_updater |
| `Database` | 8 | pf_loader + pf_updater |
| `ProgramOptions` | 4 | shared validation |
| `DailyScan` | 1 | pf_scanner |
| `StreamEodhdData`, `StreamTiingoData` | 4 | pf_streamer |
| `ResumeModeTests` | 5 | pf_streamer |

### Key Insight for Incremental Migration
The token-vector constructor (`PF_CollectDataApp(tokens)`) is the integration point. Each extracted program gets its own app class with the same `(tokens)` constructor pattern, allowing tests to migrate one fixture at a time while the monolith remains functional via delegation shims.

### Phase 3a Findings (pf_loader — completed)
- `Streamer.cpp` must be compiled into loader binary because Tiingo/Eodhd inherit from RemoteDataSource
- Loader does NOT need WebSocket/streaming functionality — only needs ATR computation helpers that use Eodhd/Tiingo HTTP APIs
- `--mode load` option removed from pf_loader CLI (fixed to load mode by design)
- First SingleFileEndToEnd test migrated successfully; second test kept on monolith (exercises both load AND update paths)

### Phase 3b Plan (pf_updater — in progress)
- Extract `Run_Update()` and `Run_UpdateFromDB()` from monolith
- Updater shares helper methods with loader: ATR computation, CSV parsing, JSON loading, shutdown persistence
- Consider shared base or utility header for loader/updater common code if duplication becomes significant

### Phase 3b Findings
- CLI11 argv-style parsing required: token values like `.1` fail when passed as string join; must use char* pointer array with proper null termination
- `--use-ATR` must use `add_flag()` not `add_option()` to avoid consuming next token as value
- `--config-dir` CLI option and `PF_COLLECT_DATA_CONFIG_DIR` env var resolution needed in loader/updater CheckArgs() for API key file lookup
- LoadAndUpdate test hang root cause: missing `--quote-port` CLI option in loader and updater. `quote_host_port_` was empty string, causing `RequestData()` to block indefinitely on DNS resolve with no port. Fix: add `--quote-port` with default `"443"` to both apps.

### Streaming Test Crash Fixes (2026-07-21)
- StreamEodhdData and ResumeModeTests crashed silently on startup — three bugs found:
  1. Missing `--symbol-list` CLI option on PF_StreamerApp (all resume tests use it). CLI11 threw ParseError before logging was configured → silent crash.
  2. No temp logger in PF_AppBase constructors — parse errors produced no visible output. Fix: stdout_color_mt temp logger in both constructors, replaced by ConfigureLogging().
  3. Vector OOB in Eodhd::GetTopOfBookAndLastClose (line 314) — `rows[1]` when ToB response has only header row (market closed). Fix: bounds check before access.
