# MiniZinc Challenge Patch Packs

This directory contains committed patch packs for compatibility-fixing old MiniZinc Challenge models after the upstream archive has been materialized into `.challenge/corpus/<archive_commit>/...`.

The sweep script applies a selected patch pack into `.challenge/patched/<archive_commit>/<patch_pack_id>/...` and leaves the raw archive snapshot untouched.

Current default pack:

- `challenge-compat-v2`
  - contains reviewable compatibility rewrites for challenge models that no longer compile unchanged on modern MiniZinc

The design intent is conservative:

- keep the raw upstream corpus untouched
- express each compatibility fix as a committed, named rule
- preserve model semantics where practical
- keep purely presentational fixes separate from semantic ones

## Rule-by-Rule Summary

### `010-remove-is_output`

Targets:

- several `2009` and `2010` models that still use `:: is_output`

What it does:

- removes the legacy `:: is_output` annotation from variable declarations

Why it exists:

- current MiniZinc uses output items directly and no longer defines `is_output`

Semantics:

- output metadata only
- no constraint change

### `020-unquote-search-annotations`

Targets:

- several `2008` models and `2009/roster`

What it does:

- rewrites old string-valued search annotations such as `"first_fail"` to modern annotation identifiers such as `first_fail`

Why it exists:

- older MiniZinc accepted search annotations in string form
- current MiniZinc expects annotation identifiers

Semantics:

- preserves the intended search annotation in modern syntax

### `030-debruijn-global-cardinality`

Targets:

- `2008/debruijn_binary`

What it does:

- replaces the old two-argument `global_cardinality(bin_code, gcc)` call with the modern three-argument form that includes an explicit value cover array

Why it exists:

- current MiniZinc requires the value cover array explicitly

Semantics:

- unchanged
- the inserted cover array is exactly `0..base-1`, which is the value set the original model intended to count

### `040-p1f-rename-local-circuit`

Targets:

- `2009/p1f`

What it does:

- renames the model's local `circuit` predicate to `p1f_circuit`

Why it exists:

- current MiniZinc already provides `circuit` in the standard library
- the local helper name collides with that stdlib predicate

Semantics:

- unchanged
- this is a name-collision fix only

### `050-search-stress2-relax-domain-asserts`

Targets:

- `2009/search_stress2`

What it does:

- removes brittle assertions that demand exact intermediate domains like `0..n`

Why it exists:

- modern MiniZinc presolve can legally narrow those domains during flattening
- the model-side assertion rejected those valid narrowed domains

Semantics:

- the structural size checks remain
- only the exact-domain assumption is removed

### `060-yumi-dynamic-empty-max`

Targets:

- `2021/yumi-dynamic`

What it does:

- guards several `max(...)` expressions so empty order tables or empty zone collections evaluate to `0` instead of aborting compilation

Why it exists:

- some challenge instances contain empty suction, fixture, or zone collections
- current MiniZinc rejects `max(empty)`

Semantics:

- preserved for non-empty collections
- empty collections get the neutral fallback `0`, which lets the intended degenerate instance shape compile

### `070-yumi-static-empty-max`

Targets:

- `2022/yumi-static`

What it does:

- adds the same kind of empty-collection guards as the dynamic variant, but only for the static model's ordering tables

Why it exists:

- some instances leave suction, gripper, or fixture ordering tables empty
- current MiniZinc rejects `max(empty)`

Semantics:

- preserved for non-empty tables
- empty tables evaluate to `0`

### `080-train-scheduling-string-interpolation`

Targets:

- `2024/train-scheduling`

What it does:

- rewrites a tuple-style interpolation fragment inside an assertion string

Why it exists:

- current MiniZinc rejects the old tuple-like interpolation syntax inside strings

Semantics:

- assertion message only
- no constraint change

### `090-cryptanalysis-array-signatures`

Targets:

- `2016/cryptanalysis`

What it does:

- updates several local predicate signatures in `step1_aes.mzn` so their declared array index order matches the arrays they are actually called with

Why it exists:

- the top-level model arrays are indexed as `[round, column, byte]`
- several local predicate declarations still used `[round, byte, column]` or truncated the round range
- current MiniZinc rejects that mismatch

Semantics:

- intended to be unchanged
- the patch only aligns local signatures with the actual model array layout
- it does not alter the equations inside the predicates

## When to Add a New Rule

Add a new committed rule when all of the following hold:

- the model fails before Atlantis meaningfully runs
- the issue is due to MiniZinc language, stdlib, or flattening compatibility drift
- the fix can be expressed as a narrow textual rewrite
- the rewrite is specific enough to review in isolation

Do not add a patch-pack rule for:

- Atlantis runtime crashes
- Atlantis propagation or search bugs
- performance-only issues that need timeout policy rather than model rewriting

Those belong in Atlantis itself or in the sweep configuration, not in the compatibility patch layer.
