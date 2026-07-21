# Agent Instructions

## Environment & Setup

- C++23 or greater required.
- Streaming hours: Monday thru Friday, 9:30 AM to 4:00 PM New York time.

- scripts folder: shell scripts.
- src folder: source code for program.
- sql_files folder: sql scripts for schema creation and testing sql.
- data_files folder: files used to populate live database.
- test_files folder: data files used for testing.
- python folder: various Python scripts which use the PF_Charts library.
- ../common_utilities: comomon includes and src for utility functions used in various modules

## Make commands

- debug build: make -f makefile_collect CFG=Debug.
- release build: make -f makefile_collect CFG=Release.
- use the -clean flag to rebuild everything.
- use the -j10 flag to speed up the build.

## Dependencies

### library for common code shared accros several modules

- ../lib_PF_Chart/libPF_Chart.a.

If you change any of the files in the ../common_utilities/ directories you will need to rebuild the libPF_Chart library.
The makefile for that library is in ../lib_PF_Chart/makefile_lib.

The command to use is: pushd && cd ../lib_PF_Chart && make -f makefile_lib CFG=Release clean && make -f makefile_lib CFG=Release -j10 && popd

## Test Drivers

- The test drivers are in ../PF_Test/.
- ../PF_Test/Unit_Test.cpp is for TDD and unit tests.
- ../PF_Test/EndToEnd_Test.cpp is for end-to-end system testing.
- both test drivers use GoogleTest.

## Domain Logic & Requirements

- **Reference Material is the Source of Truth:** The PDF documents in the `research_documents/` folder dictate exactly which fields to extract from filings, what to watch out for, and how to calculate financial ratios. Read these PDFs to formulate extraction rules and analytical computations.
- **Storage:** All permanent data must be stored in a PostgreSQL database. Use the finance DB unless told otherwise.

## Skills

Use the TDD skill to set up tests.

## Database credentials

- the postgesql database runs on localhost at port 5432.
- the point-and-figure data is stored in the (live or test) point_and_figure schema in the finance databse. use "-d finance, -U data_updater_pg".

## Database useful tables.

- PF_Chart data is stored in the pf_charts table in the (live or test) point_and_figure schema.
