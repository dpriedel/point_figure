# Progress: PF_CollectDataApp Refactoring

## Status
Phases 0-2 complete. Beginning Phase 3.

## Completed
- [x] 2026-07-17 — Analyzed codebase structure, identified god class (~2400 lines, ~52 members)
- [x] 2026-07-17 — Mapped all 28 e2e tests to target programs after refactoring
- [x] 2026-07-17 — Documented ChartDirector dependency impact (scanner exempt, other 3 link it)
- [x] 2026-07-17 — Designed 5-phase incremental migration plan with per-phase test verification
- [x] 2026-07-17 — Agreed on: 4 programs, WebSocket isolated to streamer, tests updated per phase
- [x] 2026-07-17 — Phase 0: Created `PF_AppBase` extracting CLI11 setup, logging, signal handling, DB params, config directory resolution
- [x] 2026-07-17 — Phase 0: Updated `PF_CollectDataApp` to inherit from `PF_AppBase`, removed ~300 lines of duplicated code
- [x] 2026-07-17 — Phase 0: Updated both makefiles with VPATH for `src/common/`
- [x] 2026-07-17 — Phase 0: Verified all 13 non-streaming e2e tests pass (ProgramOptions, SingleFileEndToEnd, LoadAndUpdate, Database)
- [x] 2026-07-17 — Phase 1: Created `PF_ScannerApp` in `src/scanner/`, extracted DailyScan logic
- [x] 2026-07-17 — Phase 1: Added `pf_scanner` target to makefile_collect (NO ChartDirector dependency)
- [x] 2026-07-17 — Phase 1: Updated `Database.DailyScan` test to use `PF_ScannerApp`, removed `--mode daily-scan` from tokens
- [x] 2026-07-17 — Phase 1: Updated e2e makefile with scanner sources and VPATH
- [x] 2026-07-17 — Phase 1: Verified DailyScan test passes, monolith tests still pass
- [x] 2026-07-17 — Phase 2: Created `PF_StreamerApp` in `src/streamer/`, extracted streaming logic (~14 methods, ~11 members)
- [x] 2026-07-17 — Phase 2: Fixed CLI::Range constructor type mismatch (double/int → double/double)
- [x] 2026-07-17 — Phase 2: Added `pf_streamer` target to makefile_collect (WITH ChartDirector dependency)
- [x] 2026-07-17 — Phase 2: Updated e2e makefile with streamer sources and VPATH
- [x] 2026-07-17 — Phase 2: Verified pf_streamer binary builds, e2e test binary compiles cleanly

## Pending
- [ ] Phase 3: Create `ChartProcessor`, extract `pf_loader` + `pf_updater` (13 tests migrate)
- [ ] Phase 4: Remove monolith, 4 standalone binaries

## Files Changed
### Phase 0
- `src/common/PF_AppBase.h` — new base class header
- `src/common/PF_AppBase.cpp` — new base class implementation
- `src/PF_CollectDataApp.h` — updated to inherit from PF_AppBase, removed moved members/methods
- `src/PF_CollectDataApp.cpp` — removed moved method definitions, constructors call base
- `makefile_collect` — added PF_AppBase.cpp to SRCS2, VPATH includes src/common
- `../PF_Test/makefile_e2e` — added PF_AppBase.cpp to SRCS2, VPATH includes src/common

### Phase 1
- `src/scanner/PF_ScannerApp.h` — scanner app header, inherits PF_AppBase
- `src/scanner/PF_ScannerApp.cpp` — scanner implementation with CLI options and Run_DailyScan logic
- `src/scanner/Main.cpp` — standalone scanner entry point
- `makefile_collect` — added `pf_scanner` target (no ChartDirector), unified clean rule
- `../PF_Test/EndToEnd_Test.cpp` — updated DailyScan test to use PF_ScannerApp
- `../PF_Test/makefile_e2e` — added scanner sources and VPATH

### Phase 2
- `src/streamer/PF_StreamerApp.h` — streamer app header, inherits PF_AppBase
- `src/streamer/PF_StreamerApp.cpp` — streamer implementation with Run_Streaming, PrimeChartsForStreaming, CollectStreamingData, resume methods
- `src/streamer/Main.cpp` — standalone streamer entry point
- `makefile_collect` — added `pf_streamer` target (with ChartDirector)
- `../PF_Test/makefile_e2e` — added streamer sources and VPATH

## Notes
- Full e2e test suite takes 5-10 minutes; use targeted test filters for quick verification per phase
- Streaming and resume tests excluded from quick checks (require external services or long timeouts)
- `TestProblemOptions` failure is pre-existing — monolith's streaming path returns early when market closed instead of throwing

## Reference Documents
- Research documents reviewed: `research_documents/PF_Collect_description.md`, `PF_Collect_refactoring.md`, `PF_Collect_refactoring_gemini.md`, `PF_Collect_refactoring_gemini_code.md`
- Plan incorporates suggestions from reference docs while adding: ChartDirector analysis, 4-program split (not 3), incremental migration strategy, test mapping
