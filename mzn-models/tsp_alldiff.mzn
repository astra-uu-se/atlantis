include "globals.mzn";

int: n;
array[Locations] of int: early;
array[Locations] of int: late;
set of int: Locations = 1..n;

array[Locations, Locations] of int: duration;

array[Locations] of var Locations: Sequence;

constraint all_different(Sequence);

var int: objective = sum([duration[Sequence[l], Sequence[1 + (l mod n)]] | l in Locations]);

solve minimize objective;

output ["objective: \(objective)\n\(Sequence)\n"]