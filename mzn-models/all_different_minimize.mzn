include "globals.mzn";
array [1..10] of var 1..20: x;
constraint all_different(x);
solve minimize x[1];