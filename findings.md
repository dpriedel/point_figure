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
