predicate fzn_table_int_flat(array [int] of var int: x, array [int] of int: t_flat);

predicate fzn_table_int(array [int] of var int: x, array [int, int] of int: t) =
  let {
    array[int] of int: t_flat = array1d(t);
  } in fzn_table_int_flat(x, t_flat);