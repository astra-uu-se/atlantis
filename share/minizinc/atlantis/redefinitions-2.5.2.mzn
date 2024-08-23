predicate array_var_bool_element2d_nonshifted(var int: idx1,
                                              var int: idx2,
                                              array [int,int] of var bool: x,
                                              var bool: c) = 
  let {
    int: num_rows = card(index_set_1of2(x));
    int: row_offset = min(index_set_1of2(x));
    int: col_offset = min(index_set_2of2(x));
    array[int] of var bool: x_flat = array1d(x);
    bool: all_params = is_fixed(x_flat);
  } in if all_params then 
    array_bool_element2d_nonshifted_flat(idx1, idx2, x_flat, c, num_rows, row_offset, col_offset)
  else 
    array_var_bool_element2d_nonshifted_flat(idx1, idx2, x_flat, c, num_rows, row_offset, col_offset)
  endif; 

predicate array_bool_element2d_nonshifted_flat(var int: idx1,
                                               var int: idx2,
                                               array [int] of var bool: x,
                                               var bool: c,
                                               int: num_rows,
                                               int: row_offset,
                                               int: col_offset);

predicate array_var_bool_element2d_nonshifted_flat(var int: idx1,
                                                   var int: idx2,
                                                   array [int] of var bool: x,
                                                   var bool: c,
                                                   int: num_rows,
                                                   int: row_offset,
                                                   int: col_offset);

predicate array_var_int_element2d_nonshifted(var int: idx1,
                                             var int: idx2,
                                             array [int,int] of var int: x,
                                             var int: c) = 
  let {
    int: num_rows = card(index_set_1of2(x));
    int: row_offset = min(index_set_1of2(x));
    int: col_offset = min(index_set_2of2(x));
    array[int] of var int: x_flat = array1d(x);
    bool: all_params = is_fixed(x_flat);
  } in if all_params then 
    array_int_element2d_nonshifted_flat(idx1, idx2, x_flat, c, num_rows, row_offset, col_offset)
  else 
    array_var_int_element2d_nonshifted_flat(idx1, idx2, x_flat, c, num_rows, row_offset, col_offset)
  endif;

predicate array_int_element2d_nonshifted_flat(var int: idx1,
                                              var int: idx2,
                                              array [int] of var int: x,
                                              var int: c,
                                              int: num_rows,
                                              int: row_offset,
                                              int: col_offset);

predicate array_var_int_element2d_nonshifted_flat(var int: idx1,
                                                  var int: idx2,
                                                  array [int] of var int: x,
                                                  var int: c,
                                                  int: num_rows,
                                                  int: row_offset,
                                                  int: col_offset);

predicate array_var_float_element2d_nonshifted(var int: idx1,
                                               var int: idx2,
                                               array[int,int] of var float: x,
                                               var float: c) =
  abort("float not supported");

predicate array_var_set_element2d_nonshifted(var int: idx1, var int: idx2, array[int,int] of var set of int: x, var set of int: c) =
  let {
    int: dim = card(index_set_2of2(x));
    int: min_flat = min(index_set_1of2(x))*dim+min(index_set_2of2(x))-1;
  } in array_var_set_element_nonshifted((idx1*dim+idx2-min_flat)::domain, array1d(x), c);
