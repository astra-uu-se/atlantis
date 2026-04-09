include "globals.mzn";

int: n;
set of int: Locations = 1..n;
int: depot = 1;
array[Locations] of int: early;
array[Locations] of int: late;
array[Locations, Locations] of int: duration;

array[Locations] of var Locations: Sequence;
array[Locations] of var 0..max(late): arrival;
array[Locations] of var min(early)..max(late): departure =
  [early[depot]] ++ [max(early[Sequence[l]], arrival[l]) | l in 2..n];
var int: objective = departure[n];

constraint forall (l in Locations) (
    arrival[l] = if l = 1 then
      0
    else
      departure[l - 1] + duration[Sequence[l - 1], Sequence[l]]
    endif);

constraint all_different(Sequence);

constraint Sequence[1] = depot;

constraint forall (l in Locations) (departure[l] <= late[Sequence[l]]);

solve minimize objective;

output ["objective: \(objective)\n\(Sequence)\n"]