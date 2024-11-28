include "globals.mzn";

int: numLocations;
set of int: Locations = 1..numLocations;

array[Locations, Locations] of int: Distance;

array[Locations] of var Locations: Next;

constraint circuit(Next);

var int: objective = sum([Distance[l, Next[l]] | l in Locations]);

solve minimize objective;

output ["objective: \(objective)\n\(Next)\n"]