#pragma once

// rapidcheck's g++-23 branch relies on transitive standard-library includes
// that libstdc++ currently provides and libc++ does not.
#include <algorithm>
#include <exception>
