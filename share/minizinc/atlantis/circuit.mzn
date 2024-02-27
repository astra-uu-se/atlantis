% Ensure the circuit index set always starts at 1.
predicate circuit(array[int] of var int: x) =
    let {
       int: offset = min(index_set(x))-1
    } in circuit_no_offset([ x[i] - offset | i in index_set(x) ]);

predicate circuit_no_offset(array[int] of var int: x);