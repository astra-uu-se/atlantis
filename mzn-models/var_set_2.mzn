% Solver which do not have native support for set variables should add
%   include "nosets.mzn";
% to redefinitions.mzn

int: n = 3;

var 1..n: x;
var set of 1..n: d;

constraint forall (i in 2..x) (x mod i = 0 <-> i in d);
constraint card(d) = 1;

var int: objective = x;

solve :: seq_search([
  int_search([x], input_order, indomain_min, complete),
  set_search([d], input_order, indomain_min, complete)
]) maximize objective;
