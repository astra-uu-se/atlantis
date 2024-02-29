predicate array_var_bool_element2d_nonshifted(var int: idx1,
                                              var int: idx2,
                                              array [int,int] of var bool: x,
                                              var bool: c) = 
  let {
    int: numRows = card(index_set_2of2(x));
    int: offset1 = min(index_set_1of2(x));
    int: offset2 = min(index_set_2of2(x));
    array[int] of var bool: x_flat = array1d(x);
    bool: all_params = is_fixed(x_flat);
  } in if all_params then array_bool_element2d_nonshifted_flat(idx1, idx2, x_flat, c, numRows, offset1, offset2)
    else array_var_bool_element2d_nonshifted_flat(idx1, idx2, x_flat, c, numRows, offset1, offset2) endif; 

predicate array_bool_element2d_nonshifted_flat(var int: idx1,
                                               var int: idx2,
                                               array [int] of var bool: x,
                                               var bool: c,
                                               int: numRows,
                                               int: offset1,
                                               int: offset2);

predicate array_var_bool_element2d_nonshifted_flat(var int: idx1,
                                                   var int: idx2,
                                                   array [int] of var bool: x,
                                                   var bool: c,
                                                   int: numRows,
                                                   int: offset1,
                                                   int: offset2);

predicate array_var_int_element2d_nonshifted(var int: idx1,
                                             var int: idx2,
                                             array [int,int] of var int: x,
                                             var int: c) = 
  let {
    int: numRows = card(index_set_2of2(x));
    int: offset1 = min(index_set_1of2(x));
    int: offset2 = min(index_set_2of2(x));
    array[int] of var int: x_flat = array1d(x);
    bool: all_params = is_fixed(x_flat);
  } in if all_params then array_int_element2d_nonshifted_flat(idx1, idx2, x_flat, c, numRows, offset1, offset2)
    else array_var_int_element2d_nonshifted_flat(idx1, idx2, x_flat, c, numRows, offset1, offset2) endif;

predicate array_int_element2d_nonshifted_flat(var int: idx1,
                                              var int: idx2,
                                              array [int] of var int: x,
                                              var int: c,
                                              int: numRows,
                                              int: offset1,
                                              int: offset2);

predicate array_var_int_element2d_nonshifted_flat(var int: idx1,
                                                  var int: idx2,
                                                  array [int] of var int: x,
                                                  var int: c,
                                                  int: numRows,
                                                  int: offset1,
                                                  int: offset2);

predicate array_var_float_element2d_nonshifted(var int: idx1, var int: idx2, array[int,int] of var float: x, var float: c) = abort("float not supported");
predicate array_var_set_element2d_nonshifted(var int: idx1, var int: idx2, array[int,int] of var set of int: x, var set of int: c) = abort("set not supported");

