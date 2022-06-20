% Ignore symmetry breaking constraints.
predicate symmetry_breaking_constraint(var bool: b) = true;

% Ignore redundant constraints. The effect of redundant constraints is not clear. Ignoring them will at least slightly
% reduce the number of objects to be maintained by the solver.
predicate redundant_constraint(var bool: b) = true;
