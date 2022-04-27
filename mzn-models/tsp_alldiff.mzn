include "globals.mzn";

int: numLocations;
set of int: Locations = 1..numLocations;

array[Locations, Locations] of int: Distance;

array[Locations] of var Locations: Sequence;

constraint all_different(Sequence);

var int: objective = sum([Distance[Sequence[l], Sequence[1 + (l mod numLocations)]] | l in Locations]);

solve minimize objective;

output ["objective: \(objective)\n\(Sequence)\n"]