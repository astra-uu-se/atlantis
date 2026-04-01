int: n;
int: capacity;
array[int] of int: profit;
array[int] of int: weight;
array[1..n] of var bool: included;

var 0..sum(profit): total_profit = sum([profit[i] * bool2int(included[i]) | i in 1..n]);

constraint bool_lin_le(weight, included, capacity);

solve maximize total_profit;