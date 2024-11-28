predicate fzn_circuit(array[int] of var int: x) =
    let {
       int: offset = min(index_set(x));
    } in fzn_circuit_offset(x, offset);

predicate fzn_circuit_offset(array[int] of var int: x, int: offset);