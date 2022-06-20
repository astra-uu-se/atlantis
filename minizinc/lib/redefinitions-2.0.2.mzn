% Ignore symmetry breaking constraints.
predicate symmetry_breaking_constraint(var bool: b) = true;

% Ignore redundant constraints. The effect of redundant constraints is not clear. Ignoring them will at least slightly
% reduce the number of objects to be maintained by the solver.
predicate redundant_constraint(var bool: b) = true;

predicate array_var_bool_element_nonshifted(var int: idx,
                                            array [int] of var bool: x,
                                            var bool: c) =
  let {
    int: offset = min(index_set(x));
    bool: all_params = is_fixed(x);
  } in if all_params then array_bool_element_offset(idx, x, c, offset)
    else array_var_bool_element_offset(idx, x, c, offset) endif;

predicate array_bool_element_offset(var int: idx,
                                    array [int] of var bool: x,
                                    var bool: c, int: offset);

predicate array_var_bool_element_offset(var int: idx,
                                        array [int] of var bool: x,
                                        var bool: c, int: offset);

predicate array_var_int_element_nonshifted(var int: idx,
                                           array [int] of var int: x,
                                           var int: c) =
  let {
    int: offset = min(index_set(x));
    bool: all_params = is_fixed(x);
  } in if all_params then array_int_element_offset(idx, x, c, offset)
    else array_var_int_element_offset(idx, x, c, offset) endif;

predicate array_int_element_offset(var int: idx,
                                   array [int] of var int: x,
                                   var int: c, int: offset);

predicate array_var_int_element_offset(var int: idx,
                                       array [int] of var int: x,
                                       var int: c, int: offset);