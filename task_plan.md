# Task Plan: PF_CollectDataApp Refactoring → 4 Programs

## Problem
`PF_CollectDataApp` is a ~2400-line god class with ~52 member variables handling 6 distinct run modes through a single dispatcher. Tight coupling between batch processing, database maintenance, and real-time streaming makes the code hard to maintain, test, and deploy independently.

## Goal
Split into 4 focused programs sharing a common base class:
- `pf_loader` — Initial chart creation from historical data (file or DB)
- `pf_updater` — Incremental chart updates with new price data (file or DB)
- `pf_scanner` — Daily database maintenance and trend statistics
- `pf_streamer` — Real-time WebSocket streaming during market hours

## Phases

### Phase 0 — Shared Foundation (no behavior change)
Create `PF_AppBase` extracting infrastructure shared by all programs:
- CLI11 app setup (`app_`)
- Logging configuration (`ConfigureLogging()`, logger members, log level parsing)
- Signal handling (`HandleSignal()`, `had_signal_`, `SetSignal()`, `WaitForTimer()`)
- DB params (`db_params_`)
- Config directory resolution (env var + CLI arg)

`PF_CollectDataApp` inherits from `PF_AppBase`. All existing behavior preserved.

**Files created:** `src/common/PF_AppBase.h`, `src/common/PF_AppBase.cpp`
**Tests affected:** None — all 28 e2e tests pass unchanged.
**Verification:** Debug build compiles, all e2e + unit tests green.

### Phase 1 — Extract `pf_scanner` (lowest risk)
Move to `PF_ScannerApp : public PF_AppBase`:
- `Run_DailyScan()`
- `CountChartReversalsUpAndDown()`, `CountChartTrendsContinueUpAndDown()`, `CountChartTrendsUnanimousUpAndDown()`
- `ProcessSymbolsFromDB()` (daily-scan variant)
- Scanner-specific CLI args: `--exchange-list`, `--min-dollar-volume`, `--begin-date`, `--end-date`

Keep compatibility shim in `PF_CollectDataApp::Run_DailyScan()` that delegates to scanner.

**ChartDirector:** NOT needed — pure DB operations, no graphics generation.

**Files created:** `src/scanner/PF_ScannerApp.h`, `src/scanner/PF_ScannerApp.cpp`
**Tests affected:** 1 (`DailyScan`) updated to instantiate `PF_ScannerApp`.
**Verification:** All e2e tests green. `pf_scanner` binary builds independently.

### Phase 2 — Extract `pf_streamer` (moderate risk, high isolation)
Move to `PF_StreamerApp : public PF_AppBase`:
- `Run_Streaming()`, `PrimeChartsForStreaming()`, `CollectStreamingData()`
- `StreamedDataParser()`, `ProcessUpdatesForSymbol()`, `Do_ProcessUpdatesForSymbol()`, `CollectStreamedData()`
- Resume: `LoadChartsFromFiles()`, `LoadStreamedPricesFromFiles()`, `LoadStreamedSummaryFromFile()`, `SaveStreamedPricesToFiles()`, `SaveStreamedSummaryToFile()`
- Streaming state: `streamed_prices_`, `streamed_summary_`, rate-limiting members

Move WebSocket sources to `src/streamer/` (compile ONLY into this binary):
- `Streamer.h/.cpp`, `Eodhd.h/.cpp`, `Tiingo.h/.cpp`

**ChartDirector:** Needed for real-time graphic updates. Link `-lchartdir`.

**Files created:** `src/streamer/PF_StreamerApp.h`, `src/streamer/PF_StreamerApp.cpp`
**Files moved:** `src/{Streamer,Eodhd,Tiingo}.h/.cpp` → `src/streamer/`
**Tests affected:** 9 (streaming + resume tests) updated to instantiate `PF_StreamerApp`.
**Verification:** All e2e tests green. `pf_streamer` binary builds independently with Boost.Beast + ChartDirector.

### Phase 3a — Extract `pf_loader` (done)
Move to `PF_LoaderApp : public PF_AppBase`:
- `Run_Load()`, `Run_LoadFromDB()`, `ProcessSymbolsFromDB()`
- `ComputeATRForChart()`, `ComputeATRForChartFromDB()`
- `LoadAndParsePriceDataJSON()`, `AddPriceDataToExistingChartCSV()`, `FindColumnIndex()`
- `ShutdownAndStoreOutputInFiles()`, `ShutdownAndStoreOutputInDB()`

**ChartDirector:** Needed for chart graphics generation. Link `-lchartdir`.
**Streamer.cpp:** Required — Tiingo/Eodhd inherit from RemoteDataSource.

### Phase 3b — Extract `pf_updater` (in progress)
Move to `PF_UpdaterApp : public PF_AppBase`:
- `Run_Update()`, `Run_UpdateFromDB()`
- Shares same helper methods as loader (ATR, CSV parsing, shutdown)
`ConstructChartGraphic.cpp` compiled into each binary that needs it (loader, updater, streamer). NOT moved to library — keeps library's external deps minimal and gives per-binary control over the closed-source dependency.

**Files created:** `src/updater/PF_UpdaterApp.h/.cpp`, `src/updater/Main.cpp`
**Tests affected:** 1 (`LoadAndUpdate`) migrated to `PF_UpdaterApp`.
**Verification:** pf_updater binary builds. LoadAndUpdate test fails on JSON parsing error + network timeout to Eodhd.

**Issues discovered:**
- CLI11 argv-style parsing required for token values starting with non-alphanumeric chars (e.g., `.1`)
- `--use-ATR` must use `add_flag()` not `add_option()` to prevent consuming next token
- `--config-dir` option and env var resolution needed in CheckArgs() for API key file lookup

### Phase 4 — Decommission monolith
Remove `PF_CollectDataApp`. Replace `Main.cpp` with 4 separate main files. Update makefile to produce 4 targets. Remove compatibility shims.

**Tests affected:** All tests now exercise individual programs directly.
**Verification:** Full test suite green on all 4 binaries.

## Build System Evolution

| Phase | Binaries | Notes |
|---|---|---|
| 0 | `PF_CollectData` | Unchanged |
| 1 | `PF_CollectData` + `pf_scanner` | Scanner links libpqxx, NO ChartDirector |
| 2 | `+ pf_streamer` | Streamer links Boost.Beast + ChartDirector |
| 3a | `+ pf_loader` | Loader links ChartDirector + RemoteDataSource |
| 3b | `+ pf_updater` | Updater links ChartDirector + RemoteDataSource |
| 4 | `pf_scanner`, `pf_streamer`, `pf_loader`, `pf_updater` | Monolith removed |

### ChartDirector dependency matrix
| Program | ChartDirector | Boost.Beast | libPF_Chart |
|---|---|---|---|
| `pf_scanner` | NO | NO | YES |
| `pf_streamer` | YES | YES | YES |
| `pf_loader` | YES | NO | YES |
| `pf_updater` | YES | NO | YES |

### ChartDirector notes
- Closed-source third-party library (`-lchartdir`, headers in `/usr/local/include/ChartDirector/`)
- Used in `ConstructChartGraphic.cpp` (compiled per-binary) and `PF_Chart_CD.cpp` (already in `libPF_Chart.a`)
- `ConstructChartGraphic.cpp` stays as shared source compiled into loader, updater, and streamer — NOT moved to the static library to keep per-binary control over this closed-source dependency
- `pf_scanner` is the only program without ChartDirector dependency — smaller binary, faster builds, no license surface area

## Directory Structure (after Phase 3)
```
src/
  common/
    PF_AppBase.h/.cpp            (Phase 0)
  scanner/
    PF_ScannerApp.h/.cpp         (Phase 1)
    Main.cpp                     (Phase 1)
  streamer/
    PF_StreamerApp.h/.cpp        (Phase 2)
    Main.cpp                     (Phase 2)
  loader/
    PF_LoaderApp.h/.cpp          (Phase 3a)
    Main.cpp                     (Phase 3a)
  updater/
    PF_UpdaterApp.h/.cpp         (Phase 3b)
    Main.cpp                     (Phase 3b)
  PF_CollectDataApp.h/.cpp       (removed Phase 4)
  Main.cpp                       (replaced Phase 4)
  ConstructChartGraphic.h/.cpp   (shared source, compiled per-binary into loader/updater/streamer)
  Streamer.h/.cpp                (RemoteDataSource base, compiled into loader/updater/streamer)
  Eodhd.h/.cpp                   (compiled into loader/updater/streamer)
  Tiingo.h/.cpp                  (compiled into loader/updater/streamer)
```

## Test Migration Strategy
Per phase: affected e2e tests updated to instantiate the new app class directly. Unaffected tests continue unchanged. Each phase verified green before proceeding.

| Test Fixture | Tests | Migrates In |
|---|---|---|
| `DailyScan` | 1 | Phase 1 → `PF_ScannerApp` |
| `StreamEodhdData`, `StreamTiingoData`, `ResumeModeTests` | 9 | Phase 2 → `PF_StreamerApp` |
| `SingleFileEndToEnd.VerifyCanLoadCSVDataAndSaveToChartFile` | 1 | Phase 3a → `PF_LoaderApp` |
| `SingleFileEndToEnd`, `LoadAndUpdate`, `Database`, `ProgramOptions` | 12 remaining | Phase 3b → `PF_LoaderApp` / `PF_UpdaterApp` |

## Steps
- [x] Phase 0: Create `PF_AppBase`, verify all tests pass
- [x] Phase 1: Extract `pf_scanner`, migrate 1 test
- [x] Phase 2: Extract `pf_streamer`, move WebSocket sources, migrate 9 tests
- [x] Phase 3a: Extract `pf_loader`, migrate 1 test (SingleFileEndToEnd.VerifyCanLoadCSVDataAndSaveToChartFile)
- [ ] Phase 3b: Extract `pf_updater`, debug LoadAndUpdate test (JSON parsing error + network timeout)
- [ ] Phase 4: Remove monolith, 4 standalone binaries
