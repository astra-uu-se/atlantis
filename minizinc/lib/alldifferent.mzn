predicate alldifferent(array[int] of var int: x);

predicate all_different(array[int] of var int: x) =
    alldifferent(x);