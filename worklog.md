# Worklog

## 2026-03-27

### Planned sweep
- Add portable sanitizer support to the CMake and Make build entry points.
- Run warning-focused `make build-tests`.
- Run `make build-tests-asan`.
- Run `make build-tests-ubsan`.
- Run `make build-tests-tsan`.
- Fix low-risk findings that improve robustness without changing intended behavior.

### Status
- Completed for baseline, warnings, ASan, UBSan, and TSan verification on macOS with Apple Clang.

### Completed
- Added portable sanitizer selection through `ATLANTIS_SANITIZERS` in CMake.
- Added Makefile entry points for `build-tests-{asan,ubsan,tsan}` and matching run targets.
- Verified baseline debug test build with `make build-tests`.
- Ran `ctest --output-on-failure -j 8` in `build`.
- Ran full AddressSanitizer build and test sweep.
- Ran full UndefinedBehaviorSanitizer build and test sweep.
- Ran full ThreadSanitizer build and test sweep.

### Results
- Baseline debug suite: 1079 passed, 0 failed, 6 disabled challenge tests.
- Remaining known warning noise before sanitizer runs: Apple Clang still reports 4 third-party Boost/fznparser warnings during fresh test builds.
- ASan build: succeeded with `make build-tests-asan`.
- Initial ASan suite exposed 6 failures in `InDomainTest` and `InSparseDomainTest`.
- ASan suite after fixes: 1079 passed, 0 failed, 6 disabled challenge tests.
- UBSan build: succeeded with `make build-tests-ubsan`.
- Initial UBSan runs exposed several signed-overflow and indexing issues in production code plus one UBSan-hostile mock pattern in `tElement2dVar`.
- UBSan suite after fixes: 1079 passed, 0 failed, 6 disabled challenge tests.
- Initial TSan configure attempts stalled in third-party dependency population because each build tree fetched its own CPM/FetchContent dependencies.
- TSan build: succeeded with `make build-tests-tsan` after switching to a shared CPM source cache for all build variants.
- TSan suite: 1079 passed, 0 failed, 6 disabled challenge tests.

### Fixes applied
- Fixed `test/testHelper.hpp::subsets` to:
  - handle empty input safely
  - queue the correct index for the first singleton subset
  - copy deque front values before `pop_front()` instead of keeping dangling references
- Added `include/atlantis/utils/overflow.hpp` with portable saturating helpers for add, subtract, multiply, absolute value, absolute difference, and interval size computations.
- Replaced overflow-prone interval-size arithmetic in `IntervalDomain` and `AllDifferentNode` with saturating helpers.
- Hardened `ScalarView`, `AbsDiff`, `Equal`, `EqualConst`, and `pow.hpp` against signed overflow in bound and value computations.
- Fixed `Count` to avoid overflow-prone offset indexing and corrected the `close()` upper-bound bug for `_needle`.
- Hardened `GlobalCardinalityOpen` range coverage/indexing logic against overflow and out-of-range accesses.
- Switched `Linear` recomputation to saturating arithmetic and made incremental updates fall back to full recomputation so sanitizer-safe behavior matches the invariant contract.
- Updated affected tests to assert the new saturating semantics instead of relying on undefined signed overflow.
- Reworked `tElement2dVar` integration coverage to avoid a mock pattern that triggers UBSan object-type diagnostics under Apple Clang/libc++.
- Added a shared CPM source cache default so normal, ASan, UBSan, and TSan build trees reuse the same dependency sources instead of refetching Boost/fznparser independently.

### Retest results
- Baseline debug suite: 1079 passed, 0 failed, 6 disabled challenge tests.
- ASan suite: 1079 passed, 0 failed, 6 disabled challenge tests.
- UBSan suite: 1079 passed, 0 failed, 6 disabled challenge tests.
- TSan suite: 1079 passed, 0 failed, 6 disabled challenge tests.

### Next
- Decide whether the remaining 4 Apple Clang warnings from fetched third-party Boost/fznparser code are worth suppressing locally or should remain as external noise.
- If Linux or GCC sanitizer validation is important, repeat the same sanitizer matrix there to confirm behavior outside the Apple Clang/libc++ toolchain.
