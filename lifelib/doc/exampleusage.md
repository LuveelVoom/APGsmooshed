
Example usage (Python)
----------------------

This is essentially the 'Hello World' of `lifelib`, and takes a small chaotic
pattern (called Lidka) and runs it 30000 generations in Conway's Game of Life.
It prints both the initial and final populations.

    import lifelib
    sess = lifelib.load_rules("b3s23")
    lt = sess.lifetree()
    lidka = lt.pattern("bo7b$obo6b$bo7b8$8bo$6bobo$5b2obo2$4b3o!")
    print("Initial population: %d" % lidka.population)
    lidka_30k = lidka[30000]
    print("Final population: %d" % lidka_30k.population)

This should print 13 as the initial population and 1623 as the final
population.

Now, let us walk through what happens in the code.

 - The first line imports the Python package. This is a lightweight operation
   so does not take a perceptible amount of time.

 - The second line is by far the most subtle. It takes in one or more rules
   (in this case, "b3s23" is the systematic name for Conway's Game of Life:
   birth on three neighbours and survival on two or three neighbours) and
   creates a lifelib 'session' capable of running those rules. This is a
   healthy abstraction designed to obscure what `load_rules()` _really_ does,
   which is the following:

    - Checks to see whether there is already a compiled `lifelib` shared
      library with the desired ruleset. If so, it checks its version against
      the Python package's version to ensure that it's current, and otherwise
      ignores it and proceeds:

    - Finds the highest-priority genus which supports the rule. In this case,
      it is the **b3s23life** genus, which can generate optimised C and
      assembly code to take advantage of instruction sets up to AVX-512. The
      genus is then invoked to create the rule. (If multiple rules have been
      specified, this step is repeated for each rule.)

    - Generates some extra 'glue code' to include all of the rule iterators
      into the `lifelib` source code.

    - Compiles the file `lifelib.cpp` using a C++ compiler (g++ or clang)
      into a shared library called `lifelib_b3s23.so` (even though if this
      is Windows, it's actually a DLL masquerading under the extension .so).
      This step is the most time-consuming, because the C++ compiler must
      compile tens of thousands of lines of code (C++11, C, and assembly)
      and optimise it for your machine. Typically this will take 10 or 20
      seconds to complete.

    - Dynamically loads the `lifelib_b3s23.so` shared library into the
      running process. (If you are on Windows and running outside Cygwin,
      this is loaded into a Cygwin subprocess instead, with interprocess
      communication pipes used to bridge the rift. Fortunately, this
      indirection is invisible to you, provided everything is configured
      correctly.)

 - The third line now creates a **lifetree** (hashed quadtree) in the
   session, allowed to use up to 1000 megabytes of memory before garbage
   collecting. All of our patterns reside in this lifetree, and they
   automatically take advantage of mutual compression. This means that
   if a structure occurs many times in many patterns, it will only be
   stored once in the compressed container.

 - The fourth line creates a pattern (finitely-supported configuration of
   cells inside an unbounded plane universe) called Lidka, which has 13
   live cells. The pattern is specified in a format called Run Length
   Encoded (or RLE), which is the standard for sharing patterns in cellular
   automata.

 - The fifth line reports the population of Lidka, and should print 13. The
   .population property calls lifelib code to compute the population of the
   pattern by recursively walking the quadtree.

 - The sixth line runs Lidka 30000 generations in Bill Gosper's Hashlife
   algorithm. On a modern machine, this should take less than 100 milliseconds
   owing to the speed of the b3s23life iterator. This is not done in-place,
   so a new object `lidka_30k` is returned without modifying the original
   `lidka` object.

 - The seventh line reports the population of the pattern `lidka_30k`. This
   should be exactly 1623.

That's it! You've now simulated your very first pattern in Hashlife!

The `python-lifelib` repository, along with packaging tools, contains several
[examples](https://gitlab.com/apgoucher/python-lifelib/tree/master/notebooks)
of more complex and interesting applications of `lifelib`.
