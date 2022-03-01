predicate int_max(var int: a, var int: b, var int: c) = array_int_maximum(c, [a, b]);
predicate int_min(var int: a, var int: b, var int: c) = array_int_minimum(c, [a, b]);

predicate int_le(var int: a, var int: b) = int_lin_le([1, -1], [a, b], 0);
predicate int_lt(var int: a, var int: b) = int_lin_le([1, -1], [a, b], -1);

predicate int_ne(var int: a, var int: b) = int_lin_ne([1, -1], [a, b], 0);
predicate int_plus(var int: a, var int: b, var int: c) = int_lin_eq([1, 1, -1], [a, b, c], 0);

predicate set_in(var int: x, set of int: S) = abort("Set variables are unsupported");