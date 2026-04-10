predicate fzn_table_int_flat_reif(array [int] of var int: x, array [int] of int: t_flat, var bool: r);

predicate fzn_table_int_reif(array [int] of var int: x, array [int, int] of int: t, var bool: r) =
  let {
    array[int] of int: t_flat = array1d(t);
  } in fzn_table_int_flat_reif(x, t_flat, r);
