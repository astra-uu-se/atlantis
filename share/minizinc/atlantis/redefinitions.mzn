%%%%%%%%%%%%%%%%%%%
% Set constraints %
%%%%%%%%%%%%%%%%%%%
predicate array_set_element(var int: b, array [int] of set of int: as, var set of int: c)
    = abort("Set variables are unsupported");

predicate array_var_set_element(var int: b, array [int] of var set of int: as, var set of int: c)
    = abort("Set variables are unsupported");

predicate set_card(var set of int: S, var int: x)
    = abort("Set variables are unsupported");

predicate set_diff(var set of int: x, var set of int: y, var set of int: r)
    = abort("Set variables are unsupported");

predicate set_eq(var set of int: x, var set of int: y)
    = abort("Set variables are unsupported");

predicate set_eq_reif(var set of int: x, var set of int: y, var bool: r)
    = abort("Set variables are unsupported");

predicate set_in(var int: x, var set of int: S)
    = abort("Set variables are unsupported");

predicate set_in_reif(var int: x, var set of int: S, var bool: r)
    = abort("Set variables are unsupported");

predicate set_intersect(var set of int: x, var set of int: y, var set of int: r)
    = abort("Set variables are unsupported");

predicate set_le(var set of int: x, var set of int: y)
    = abort("Set variables are unsupported");

predicate set_le_reif(var set of int: x, var set of int: y, var bool: r)
    = abort("Set variables are unsupported");

predicate set_lt(var set of int: x, var set of int: y)
    = abort("Set variables are unsupported");

predicate set_lt_reif(var set of int: x, var set of int: y, var bool: r)
    = abort("Set variables are unsupported");

predicate set_ne(var set of int: x, var set of int: y)
    = abort("Set variables are unsupported");

predicate set_ne_reif(var set of int: x, var set of int: y, var bool: r)
    = abort("Set variables are unsupported");

predicate set_subset(var set of int: x, var set of int: y)
    = abort("Set variables are unsupported");

predicate set_subset_reif(var set of int: x, var set of int: y, var bool: r)
    = abort("Set variables are unsupported");

predicate set_superset(var set of int: x, var set of int: y)
    = abort("Set variables are unsupported");

predicate set_superset_reif(var set of int: x, var set of int: y, var bool: r)
    = abort("Set variables are unsupported");

predicate set_symdiff(var set of int: x, var set of int: y, var set of int: r)
    = abort("Set variables are unsupported");

predicate set_union(var set of int: x, var set of int: y, var set of int: r)
    = abort("Set variables are unsupported");

%%%%%%%%%%%%%%%%%%%%%
% Float constraints %
%%%%%%%%%%%%%%%%%%%%%
predicate array_float_element(var int: b, array [int] of float: as, var float: c)
    = abort("Float variables are unsupported");

predicate array_float_maximum(var int: m, array [int] of var int: x)
    = abort("Float variables are unsupported");

predicate array_float_minimum(var int: m, array [int] of var int: x)
    = abort("Float variables are unsupported");

predicate array_var_float_element(var int: b, array [int] of var float: as, var float: c)
    = abort("Float variables are unsupported");

predicate float_abs(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_acos(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_acosh(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_asin(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_asinh(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_atan(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_atanh(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_cos(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_cosh(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_div(var float: a, var float: b, var float: c)
    = abort("Float variables are unsupported");

predicate float_eq(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_eq_reif(var float: a, var float: b, var bool: r)
    = abort("Float variables are unsupported");

predicate float_exp(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_le(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_le_reif(var float: a, var float: b, var bool: r)
    = abort("Float variables are unsupported");

predicate float_lin_eq(array [int] of float: as, array [int] of var float: bs, float: c)
    = abort("Float variables are unsupported");

predicate float_lin_eq_reif(array [int] of float: as, array [int] of var float: bs, float: c, var bool: r)
    = abort("Float variables are unsupported");

predicate float_lin_le(array [int] of float: as, array [int] of var float: bs, float: c)
    = abort("Float variables are unsupported");

predicate float_lin_le_reif(array [int] of float: as, array [int] of var float: bs, float: c, var bool: r)
    = abort("Float variables are unsupported");

predicate float_lin_lt(array [int] of float: as, array [int] of var float: bs, float: c)
    = abort("Float variables are unsupported");

predicate float_lin_lt_reif(array [int] of float: as, array [int] of var float: bs, float: c, var bool: r)
    = abort("Float variables are unsupported");

predicate float_lin_ne(array [int] of float: as, array [int] of var float: bs, float: c)
    = abort("Float variables are unsupported");

predicate float_lin_ne_reif(array [int] of float: as, array [int] of var float: bs, float: c, var bool: r)
    = abort("Float variables are unsupported");

predicate float_ln(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_log10(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_log2(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_lt(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_lt_reif(var float: a, var float: b, var bool: r)
    = abort("Float variables are unsupported");

predicate float_max(var float: a, var float: b, var float: c)
    = abort("Float variables are unsupported");

predicate float_min(var float: a, var float: b, var float: c)
    = abort("Float variables are unsupported");

predicate float_ne(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_ne_reif(var float: a, var float: b, var bool: r)
    = abort("Float variables are unsupported");

predicate float_plus(var float: a, var float: b, var float: c)
    = abort("Float variables are unsupported");

predicate float_pow(var float: x, var float: y, var float: z)
    = abort("Float variables are unsupported");

predicate float_sin(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_sinh(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_sqrt(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_tan(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_tanh(var float: a, var float: b)
    = abort("Float variables are unsupported");

predicate float_times(var float: a, var float: b, var float: c)
    = abort("Float variables are unsupported");

predicate int2float(var int: x, var float: y)
    = abort("Float variables are unsupported");
