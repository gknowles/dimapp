<!--
Copyright Glen Knowles 2021 - 2025.
Distributed under the Boost Software License, Version 1.0.
-->

# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com)
and this project adheres to [Semantic Versioning](http://semver.org).

## Unreleased (2026-02-??)
- Added - Process code files into site pages
- Added - Variables and their inclusion in docgen.xml
- Added - Groups of pages with table of contents for the group
- Fixed - Crash making tests when requested layout not found

## docgen 1.5.0 (2025-02-20)
- Fixed - Honor non-default layouts from older tagged versions
- Added - Arbitrary site wide files

## docgen 1.4.0 (2024-07-11)
- Cosmetic - Update bootstrap to 4.6.2 and highlistjs to 11.9.0
- Fixed - Race condition detecting child process completions
- Added - Report git errors loading content
- Fixed - Race condition adding files to output set
- Added - favicon.ico

## docgen 1.3.1 (2024-04-19)
- Fixed - Memory leak after compilation errors.

## docgen 1.3.0 (2023-03-26)
- Added - Auto-testing for samples in markdown files

## docgen 1.2.1 (2022-02-10)
- Fixed - Hangs after reporting web site generation errors

## docgen 1.2.0 (2022-01-14)
- Fixed - Last embedded test of documents not processed
- Fixed - Memory leak when first compile fails
- Added - Option to let grandchild continue after child process stopped

## docgen 1.1.0 (2021-09-06)
- Added - Command line --no-update and --no-compile options
- Added - Attribute "test repl" for scripts
- Added - More diagnostics parsing tests from documentation

## docgen 1.0.0 (2021-08-16)
First public release
