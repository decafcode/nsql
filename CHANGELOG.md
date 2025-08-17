# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- Adopt keepachangelog.com CHANGELOG format

## [2.4.0] - 2022-01-15

### Changed

- Update SQLite to v3.37.1
- Update node-gyp dependency to v6.1.0

## [2.3.1] - 2021-09-06

### Changed

- Update SQLite version number in README

## [2.3.0] - 2021-09-06

### Changed

- Update SQLite to v3.36.0

## [2.2.0] - 2021-03-07

### Changed

- Update SQLite to v3.34.1
- Build for NAPI v6
- Minor documentation updates

## [2.1.0] - 2020-05-23

### Changed

- Update SQLite to v3.31.1

### Fixed

- Fix INTERNALS.md link in README

## [2.0.0] - 2020-01-05

### Changed

- Return SQLite extended error codes in `code` property of thrown exceptions
  - This is an API-breaking change
- Test suite enhancements

### Fixed

- Fix diagnostics returned by `Database.prepare()`
- Check for closed database handles in various methods
- Handle statement reset failure more gracefully

## [1.0.0] - 2019-12-28

### Added

- Initial release
