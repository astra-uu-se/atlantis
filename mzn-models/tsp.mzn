include "globals.mzn";

int: n;
set of int: Locations = 1..n;

array[Locations, Locations] of int: duration;

array[Locations] of var Locations: Next;

constraint circuit(Next);

var int: objective = sum([duration[l, Next[l]] | l in Locations]);

solve minimize objective;

output ["objective: \(objective)\n\(Next)\n"]