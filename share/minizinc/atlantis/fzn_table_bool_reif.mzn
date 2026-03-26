predicate fzn_table_bool_flat_reif(array [int] of var bool: x, array [int] of bool: t_flat, var bool: r);

predicate fzn_table_bool_reif(array [int] of var bool: x, array [bool, bool] of int: t, var bool: r) =
  let {
    array[int] of int: t_flat = array1d(t);
  } in fzn_table_int_flat(x, t_flat, r);