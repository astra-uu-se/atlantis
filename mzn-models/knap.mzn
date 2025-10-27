int: n;
int: capacity;
array[int] of int: profit;
array[int] of int: weight;
array[1..n] of var 0..1: included;

var 0..sum(profit): total_profit = sum([profit[i] * included[i] | i in 1..n]);
var 0..capacity: total_weight = sum([weight[i] * included[i] | i in 1..n]);

solve maximize total_profit;