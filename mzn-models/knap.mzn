int: n;
int: capacity;
array[int] of int: profit;
array[int] of int: weight;
array[1..n] of var 0..1: included;

var 0..sum(profit): total_profit = sum([profit[i] * included[i] | i in 1..n]);

constraint sum([weight[i] * included[i] | i in 1..n]) <= capacity;

solve maximize total_profit;