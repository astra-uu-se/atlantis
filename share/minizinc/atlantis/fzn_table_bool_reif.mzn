predicate fzn_table_bool_flat_reif(array [int] of var bool: x, array [int] of bool: t_flat, var bool: r);

predicate fzn_table_bool_reif(array [int] of var bool: x, array [int, int] of bool: t, var bool: r) =
  let {
    array[int] of bool: t_flat = array1d(t);
  } in fzn_table_bool_flat_reif(x, t_flat, r);
