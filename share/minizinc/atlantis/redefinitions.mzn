% We don't support set variables:
include "nosets.mzn";
% add the annotation for computed domains. The domain constraint for a variable 
% with the computed domain annotation can be ignored (no domain violation 
% invariant has to be added to the invariant graph):
% mzn_opt_annotate_computed_domains = true;
% Ignore symmetry breaking constraints: 
mzn_ignore_symmetry_breaking_constraints=true;
%
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
