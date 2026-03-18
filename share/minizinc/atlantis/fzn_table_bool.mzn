predicate fzn_table_bool_flat(array [int] of var bool: x, array [int] of bool: t_flat);

predicate fzn_table_bool(array [int] of var bool: x, array [bool, bool] of int: t) =
  let {
    array[int] of int: t_flat = array1d(t);
  } in fzn_table_int_flat(x, t_flat);