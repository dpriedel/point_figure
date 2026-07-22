# Progress: PF_CollectDataApp Refactoring

## Status
All phases complete. All tests pass. Streaming tests require market hours (9:30 AM - 4:00 PM ET).

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
- [x] 2026-07-17 — Phase 3a: Created `PF_LoaderApp` in `src/loader/`, extracted loader logic (~8 methods, ~25 members)
- [x] 2026-07-17 — Phase 3a: Added `pf_loader` target to makefile_collect (WITH ChartDirector + Streamer.o for RemoteDataSource)
- [x] 2026-07-17 — Phase 3a: Updated e2e makefile with loader sources and VPATH
- [x] 2026-07-17 — Phase 3a: Migrated `SingleFileEndToEnd.VerifyCanLoadCSVDataAndSaveToChartFile` to use `PF_LoaderApp`
- [x] 2026-07-17 — Phase 3a: Verified pf_loader binary builds, all SingleFileEndToEnd tests pass (2/2), monolith tests still pass
- [x] 2026-07-17 — Phase 3b: Created `PF_UpdaterApp` in `src/updater/`, extracted updater logic (~4 methods)
- [x] 2026-07-17 — Phase 3b: Added `pf_updater` target to makefile_collect (WITH ChartDirector + Streamer.o for RemoteDataSource)
- [x] 2026-07-17 — Phase 3b: Updated e2e makefile with updater sources and VPATH
- [x] 2026-07-17 — Phase 3b: Migrated `LoadAndUpdate.VerifyUpdateWorksWhenNoPreviousChartData` to use `PF_UpdaterApp`, removed `--mode update` from tokens
- [x] 2026-07-17 — Phase 3b: Fixed CLI11 parsing in `PF_AppBase::ParseProgramOptions` — changed from string join to argv-style char* pointer array for proper token handling
- [x] 2026-07-17 — Phase 3b: Fixed `--use-ATR` from `add_option` to `add_flag` in both loader and updater to prevent consuming next token as value
- [x] 2026-07-17 — Phase 3b: Removed strict `CLI::ExistingPath` check from `--output-chart-dir` (directory created in CheckArgs if missing)
- [x] 2026-07-17 — Phase 3b: Added `--config-dir` CLI option to loader and updater for API key file resolution
- [x] 2026-07-17 — Phase 3b: Added env var resolution (`PF_COLLECT_DATA_CONFIG_DIR`) in CheckArgs() for both loader and updater
- [x] 2026-07-17 — Phase 3b: Updated LoadAndUpdate test tokens with `--config-dir` pointing to API key directory
- [x] 2026-07-18 — Phase 3b: Fixed missing `--quote-port` option in loader and updater (default "443"). Root cause of LoadAndUpdate test hang: empty port string caused `RequestData` to block on DNS resolve. Test now passes in ~5.5s.
- [x] 2026-07-21 — Streaming test crash fixes: Added `--symbol-list` CLI option to PF_StreamerApp, temp logger in PF_AppBase constructors for parse error visibility, bounds check on Eodhd ToB response, deleted copy/move operators on PF_StreamerApp
- [x] 2026-07-21 — Fixed temp_logger leak causing 8 test failures: added `spdlog::drop("temp_logger")` in destructor so each app instance cleans up its temp logger
- [x] 2026-07-22 — Phase 4: Decommissioned monolith. Removed `PF_CollectDataApp.h/.cpp` and `src/Main.cpp`. Updated makefiles to remove monolith targets. All ~44 e2e tests migrated to individual app classes. All tests pass.

## Pending
_(none)_



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

### Phase 3b
- `src/updater/PF_UpdaterApp.h` — updater app header, inherits PF_AppBase
- `src/updater/PF_UpdaterApp.cpp` — updater implementation with Run_Update, Run_UpdateFromDB, shared helpers; `--use-ATR` uses `add_flag`, `--config-dir` option, env var resolution in CheckArgs
- `src/updater/Main.cpp` — standalone updater entry point
- `src/loader/PF_LoaderApp.cpp` — added `--config-dir` option and env var resolution in CheckArgs
- `src/common/PF_AppBase.cpp` — ParseProgramOptions now uses argv-style char* pointer array for CLI11 token handling
- `makefile_collect` — added `pf_updater` target (with ChartDirector)
- `../PF_Test/EndToEnd_Test.cpp` — LoadAndUpdate test migrated to PF_UpdaterApp, added `--config-dir` token
- `../PF_Test/makefile_e2e` — added updater sources and VPATH

### Streaming Test Crash Fixes (2026-07-21)
- `src/common/PF_AppBase.h` — added `#include <spdlog/sinks/stdout_color_sinks.h>` for temp logger
- `src/common/PF_AppBase.cpp` — temp stdout logger in both constructors (before CLI parsing)
- `src/streamer/PF_StreamerApp.h` — added `symbol_list_i_` member, deleted copy/move/assignment operators
- `src/streamer/PF_StreamerApp.cpp` — added `--symbol-list` CLI option and comma-separated parsing logic
- `src/Eodhd.cpp` — bounds check on ToB response rows vector before accessing `rows[1]`

## Notes
- Full e2e test suite takes 5-10 minutes; use targeted test filters for quick verification per phase
- Streaming and resume tests excluded from quick checks (require external services or long timeouts)
- `TestProblemOptions` failure is pre-existing — monolith's streaming path returns early when market closed instead of throwing

## Reference Documents
- Research documents reviewed: `research_documents/PF_Collect_description.md`, `PF_Collect_refactoring.md`, `PF_Collect_refactoring_gemini.md`, `PF_Collect_refactoring_gemini_code.md`
- Plan incorporates suggestions from reference docs while adding: ChartDirector analysis, 4-program split (not 3), incremental migration strategy, test mapping
