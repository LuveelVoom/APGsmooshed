
Editing features
----------------

In addition to running patterns, `lifelib` has extensive support for editing
patterns. In particular, the following operations are included:

 - Shifting, rotating, and reflecting patterns.
 - Boolean set operations such as union, intersection, difference, symmetric
   difference.
 - Slicing rectangular subregions.
 - Convolutions (either using inclusive or exclusive disjunction).
 - Getting and setting individual cells or arrays thereof.
 - Pattern-matching capabilities such as find and replace.
 - Kronecker products.

The Python interface allows arbitrarily large integers to be used for shifts
and getting/setting coordinates of individual cells. Manipulating arrays of
cells is restricted to 64-bit integers, and requires `numpy` to be installed,
but is much faster.

Both the Python and C++ versions of `lifelib` use operator overloading for
editing patterns, and are consistent with each other:

 - Two patterns can be added (elementwise bitwise OR) by using `|` or `+`.
   The in-place assignment operators `|=` and `+=` also work. For two-state
   patterns viewed as sets of cells, this coincides with union / disjunction.

 - Patterns can be subtracted (elementwise bitwise AND-NOT) by using `-` or
   its in-place form `-=`.

 - The intersection / conjunction (elementwise bitwise AND) is exposed
   through the operator `&` and its in-place form `&=`.

 - Exclusive disjunction can be performed using the caret operator `^` and
   its in-place form `^=`.

 - The shift operators `<<` and `>>` apply elementwise to cell states, where
   the word size is equal to the number of layers in the parent lifetree.

 - The Kronecker product of two patterns can be performed using `*`.

 - A pattern may be shifted using either `pattern.shift(30, -20)` or the
   more concise `pattern(30, -20)`. The latter syntax can have a third
   parameter prepended to allow transformations, such as counter-clockwise
   rotation using `pattern("rccw", 0, 0)`.

 - The square brackets operator advances a pattern in the current rule for
   the specified number of generations.

In Python, a pattern can also be viewed as an associative array mapping
coordinate pairs to the state, and manipulated as such. The `__getitem__`
and `__setitem__` methods (square brackets operator as an rvalue and lvalue
respectively) support the following:

    lidka[-3:5, 8:20] # for specifying an 8-by-12 rectangular region
    lidka[7893, -462] # for specifying an individual cell
    lidka[np.array([[3, 4], [2, 1], [69, -42]])] = np.array([1, 0, 1])

The first of these returns a pattern and supports assignment from either a
pattern or an int (in which case it block-fills the rectangle with that
state). If you provide a dictionary such as `{0: 0.60, 1: 0.20, 2: 0.20}`,
then it will fill the rectangle randomly with cells of states given by the
keys in the dictionary, and values in the proportions in the dictionary.
For two-state random fill, you can use:

    pat[0:100, 0:100] = 0.3

to fill that rectangle with 30-percent density.

The second notation returns an int and supports assignment from an int. The
third returns and supports assignment from a numpy array of the same length.
The numpy array syntax can only specify 64-bit signed coordinates (each
coordinate is limited to the interval $`\pm 9 \times 10^{18}`$ ), whereas
the other two syntaxes allow pairs of arbitrarily large ints to be used.
