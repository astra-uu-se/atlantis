include "globals.mzn";
% Tile data type:
type TileData = record(int: u, int: r, int: d, int: l);
% Parameters
int: h; % Board height
int: w; % Board width
set of int: Patterns; % The diffent patterns appearing on the tiles
set of int: Rows = 1..h;
set of int: Cols = 1..w;
set of int: VertBor = 2..h-1;
set of int: HoriBor = 2..w-1;
int: numBor = 2*(h-2 + w-2); % number of border (non-corner frame) tiles
int: numCen = (h-2)*(w-2);   % number of center tiles

array[1..4,     1..4] of TileData: Cor; % Corner tiles
array[1..numBor,1..4] of TileData: Bor; % Border tiles
array[1..numCen,1..4] of TileData: Cen; % Center tiles
% Each tile has an id and an uo, right, down, and left pattern:
type TileVar = var record(1..max([4,numBor,numCen]): id,
                          Patterns: u, Patterns: r,
                          Patterns: d, Patterns: l);
%% Decision variables:
array[Rows,Cols] of TileVar: T; % T[y,x] = tile at row y, column x:
% O[y,x] = orientation of tile at row y, column x:
array[2..h-1,2..w-1] of var 1..4: O; 
% Number of incorrectly matched patterns:
var 0..(h-1)*w+h*(w-1): gaps =
  sum(y in Rows, x in 1..w-1 )(T[y,x].r != T[y,x+1].l) + 
  sum(y in 1..h-1, x in Cols)(T[y, x].d != T[y+1,x].u);
%%% Constraints:
% Tighten domain of the tile ids:
constraint forall(y in {1,h},   x in {1,w})  (T[y,x].id in 1..4);
constraint forall(y in VertBor, x in {1,w})  (T[y,x].id in 1..numBor);
constraint forall(y in {1,w},   x in HoriBor)(T[y,x].id in 1..numBor);
constraint forall(y in VertBor, x in HoriBor)(T[y,x].id in 1..numCen);
% Every corner, border, and center tile may appear only once:
constraint alldifferent([T[y,x].id | y in {1,h}, x in {1,w}]);
constraint alldifferent([T[y,x].id | y in {1,h}, x in HoriBor] ++ 
                        [T[y,x].id | y in VertBor, x in {1,h}]);
constraint alldifferent([T[y,x].id | y in VertBor, x in HoriBor]);
% conFrame(y,x,V) constrains the border tile T[y,x] to be a valid:
predicate conFrame(int: y, int: x, array[int] of TileData: V) = 
  let {
    array[int] of int: u = [t.u | t in V];
    array[int] of int: r = [t.r | t in V];
    array[int] of int: d = [t.d | t in V];
    array[int] of int: l = [t.l | t in V];
  } in
    T[y,x].u = if y = 1 then 0 else u[T[y,x].id] endif /\ 
    T[y,x].r = if x = w then 0 else r[T[y,x].id] endif /\ 
    T[y,x].d = if y = h then 0 else d[T[y,x].id] endif /\ 
    T[y,x].l = if x = 1 then 0 else l[T[y,x].id] endif;
% Enforce each center tile T[y,x] with orientation O[y,x] to be valid:
constraint let {
  array[int,int] of int: u = array2d(1..numCen,1..4,[t.u | t in Cen]);
  array[int,int] of int: r = array2d(1..numCen,1..4,[t.r | t in Cen]);
  array[int,int] of int: d = array2d(1..numCen,1..4,[t.d | t in Cen]);
  array[int,int] of int: l = array2d(1..numCen,1..4,[t.l | t in Cen]);
  } in forall (y in HoriBor, x in VertBor) (
    T[y,x].u = u[T[y,x].id,O[y,x]] /\ 
    T[y,x].r = r[T[y,x].id,O[y,x]] /\ 
    T[y,x].d = d[T[y,x].id,O[y,x]] /\ 
    T[y,x].l = l[T[y,x].id,O[y,x]]);
% Enforce each corner tile to be valid:
constraint forall(x in {1, w}, y in {1, h}) ( let { 
    array[int] of TileData: V = [t | t in Cor where 
      if y = 1 then t.u = 0 else t.d = 0 endif /\
      if x = 1 then t.l = 0 else t.r = 0 endif];
  } in conFrame(y,x,V));
% Enforce each top and bottom border tile to be valid:
constraint forall(y in {1, h}) (let {
    array[int] of TileData: V = [t | t in Bor where 
      if y = 1 then t.u = 0 else t.d = 0 endif];
  } in forall(x in HoriBor) (conFrame(y,x,V)));
% Enforce each left and right border tile to be valid:
constraint forall(x in {1, w}) (let {
    array[int] of TileData: V = [t | t in Bor where 
      if x = 1 then t.l = 0 else t.r = 0 endif];
  } in forall(y in VertBor) (conFrame(y,x,V)));
%% Symmetry-breaking constraints
% Rotational and reflection symmetry:
constraint symmetry_breaking_constraint(
  T[1,1].id < T[1,w].id /\ T[1,1].id < T[h,1].id /\
  T[1,1].id < T[h,w].id);
solve
  :: seq_search([
      int_search([T[y,x].id | y in {1,h}, x in {1,w}], first_fail, indomain_min),  % Start by placing corners
      int_search([T[y,w].id | y in 2..h-1, x in {1,h}] ++ 
                 [T[y,w].id | y in {1,h}, x in 2..w-1], first_fail, indomain_min), % Then place borders
      int_search([T[y,w].id | y in 2..h-1, x in 2..w-1], first_fail, indomain_min) % Finally place remaining pieces
    ])
  minimize gaps;

array[0..26] of string: A = array1d(0..26,[" ", "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"]);
output  [    if i = 1 then " " ++ A[fix(T[y,x]).u] ++ " "
         elseif i = 2 then A[fix(T[y,x].l)] ++ "+" ++ A[fix(T[y,x].r)]
         else              " " ++ A[fix(T[y,x].d)] ++ " " endif ++
             if x = w then "\n" else "" endif | y in 1..h, i in 1..3, x in 1..w] ++
        ["obj: \(gaps)\n"];