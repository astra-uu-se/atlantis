/***
!Test
type: compile
solvers: [gecode]
expected: !FlatZinc comp_domain_ann.fzn
***/

array[1..4] of var 1..4: x;
constraint sum(x) = product(x);
