include "globals.mzn";
int: numClasses;
int: numOptions;
int: numCars;

array[Classes] of int: ClassCount;
array[Classes, Options] of int: CarData;
array[Options] of int: MaxCarsInBlock;
array[Options] of int: BlockSize;

set of int: Classes = 1..numClasses;
set of int: Options = 1..numOptions;
set of int: Cars = 1..numCars;

array[Cars] of int: CarClass = [class | class in Classes, car in 1..ClassCount[class]];


array[Cars, Options] of int: CarOption = array2d(
  Cars,
  Options,
  [CarData[class, option] | class in Classes, car in 1..ClassCount[class], option in Options]
);

array[Cars] of var Cars: Sequence;

constraint all_different(Sequence);

constraint forall(o in Options) (
  forall(start in 1..(numCars - BlockSize[o] + 1)) (
    sum([CarOption[Sequence[car], o] | car in start..(start+BlockSize[o] - 1)]) <= MaxCarsInBlock[o]
  )
);

output ["Car Class\t Options Req\n"] ++
       [join("\n", ["\(CarClass[car])\t" ++ join(" ", ["\(CarOption[car, o])" | o in Options]) | car in Sequence])];