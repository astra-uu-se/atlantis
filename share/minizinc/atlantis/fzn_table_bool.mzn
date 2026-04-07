predicate fzn_table_bool_flat(array [int] of var bool: x, array [int] of bool: t_flat);

predicate fzn_table_bool(array [int] of var bool: x, array [int, int] of bool: t) =
  let {
    array[int] of bool: t_flat = array1d(t);
  } in fzn_table_bool_flat(x, t_flat);
