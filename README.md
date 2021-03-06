# ducts
This is a record of my attempts at Quora's Datacenter Cooling problem.

I originally saw the problem on Quora's careers page.
For reference, I found it again [here](http://www.businessinsider.com/heres-the-test-you-have-to-pass-to-work-at-quora-silicon-valleys-hot-new-86-million-startup-2010-4).


## Project Status
It's time for me to move on.  Working on this has definitely helped me get back into the swing of working with architecting, writing, testing, debugging, etc., after two years away from the industry.  But now I feel like I'm just grinding away at details.  Time to move on to something else!


## How to run
`compile.sh` compiles with `gcc -O3 -oducts ducts.c`.

`test_cases/` directory contains test cases and expected results.  `inputa` and `inputb` are the small and big examples given in the original problem statement.  The other examples fall somewhere in between.

`run_test.sh` runs the test cases.  Test output goes to stdout as well as the `test_output` file.  You can pass a list of test cases if you don't want to run them all: `./run_test.sh test_cases/input{1,2,3}`.  The corresponding output files are automatically used to check the results.

`run_perf_test.sh` runs each test case ten times and outputs the average run time (centi-second precision).  Only counts user time, as system time is negligible (on the order of 0.01%).  Test output goes to stdout as well as the `perf_test_output` file.  You can pass a list of test cases if you don't want to run them all: `./run_perf_test.sh test_cases/input1 test_cases/input2`.  The output is ignored (not checking for correctness).


## Performance
Not including input1, input2, or inputa, because runtimes are always less than 0.00s.
Not including versions that didn't change performance.
\* indicates the test was manually run twice and the times were averaged, rather than
using `run_perf_test.sh` (which runs ten times, which would take forever).

| Ver | input3 (s) | input4 | input5 | inputb    | Comments
|----:|-----------:|-------:|-------:|----------:|:--------
| 1.0 |       0.23 |  25.36 | 149.32 | \*4863.61 | First attempt
| 2.0 |       0.41 |  44.62 | 264.50 | \*8815.87 | Replace array with 64-bit variable
| 2.1 |       0.39 |  40.34 | 235.78 | \*8137.41 | Performance bug fix
| 3.0 |       0.16 |  18.72 | 114.33 | \*3952.12 | Replace position with 64-bit variable
| 4.0 |       0.05 |   1.90 |   9.77 |     89.28 | Flood-fill check... Bam!
| 4.6 |       0.02 |   0.89 |   3.67 |     32.82 | Fixed flood-fill bug
| 4.9 |       0.02 |   0.69 |   2.82 |     25.75 | Ifdeffed out stats and progress printing


## Version history
| Ver | Comments
|----:| :----
| 1.0 | Initial release
| 1.1 | Added this block of comments
| 2.0 | Using a 64-bit `uint64_t` instead of an array to store the room structure. This limits `width*length` to 64. Credit for the idea to [Pathikrit Bhowmick](https://github.com/pathikrit/Quora-Challenges/). The original problem description doesn't include any size limits, but I still feel silly for not thinking of this myself.
| 2.1 | Performance bug fix: No longer continuing to search after reaching the end room.  But it turns out, this is still slower than version 1.  I had thought copying the whole room structure (as a single `uint64_t`) onto the stack when recursing would help avoid data hazards in the processor.  But it seems there are much bigger bottlenecks: possibly the function calls themselves, branching, and multiplication in position calculation.  Will have to give the compiler some more help!  Perhaps handling the search in a loop instead of a recursive function call would help reduce dependencies?  Also, now that the array structure is copied at every search level, the search could easily be given to several threads.
| 3.0 | 3.0 Replace position struct with a `uint64_t` bitmask into the room structure. Exactly one bit is set in a position bitmask at any time. Finally, something faster than version 1! But only about 25% faster.
| 3.1 | Simplified search2() with a (kind of complicated) macro. Performance unchanged. This let me play with macros, but it would be interesting to do the same thing with a helper function and some indirection. I wonder how much the compiler could optimize it away.
| 4.0 | Added periodic checking for empty rooms that are cut off, using a recursive flood-fill algorithm. The flood-fill is *much* less expensive than the path search, so avoiding some large dead-end branches gives a huge performance gain. This is another idea from [Pathikrit Bhowmick](https://github.com/pathikrit/Quora-Challenges/). I added a heuristic based on datacenter size to determine how early to start using these flood-fills.
| 4.1 | Added forward declarations, moved functions and global variable definitions, and fixed up comments/formatting.  Sorry for the messy diff, but rest assured there are no material changes.
| 4.2 | Added `scanf_int_safe()`, a scanf wrapper to handle error checking and reporting.
| 4.3 | Added macros to replace hardcoded values.
| 4.4 | Replaced `call_with_param()` macro with `call_with_params()` macro (takes variable number of parameters).  Just to play with `__VA_ARGS__` in macros `^_^`.  Maybe I will use the added functionality later.
| 4.5 | Added some methods to handle input, setup, and debug output.
| 4.6 | Fixed another performance bug. `try_flood()` no longer returns success when exactly one room is cut off. This does not affect the results, but fixing it gives a huge performance boost.
| 4.7 | Fixed error message formatting.
| 4.8 | Bugfix: Now works for 8x8 datacenters.
| 4.9 | Used `#ifdef WITH_STATS_AND_PROGRESS` to exclude that code unless it is explicitly desired.  20% to 25% percent performance gain.
| 5.0 | Bugfix: Now correctly handles 64-room inputs. Also fixed scanf error checking, and added a check for input with more than 64 rooms.  Added test cases for large inputs.



## Tests (current version)

### Performance test results
```
$ ./run_perf_test.sh
Wed Sep 28 17:42:43 PDT 2016
test_cases/input1 avg time: 0
test_cases/input2 avg time: 0
test_cases/input3 avg time: .02
test_cases/input4 avg time: .69
test_cases/input5 avg time: 2.82
test_cases/input10 avg time: 2.15
test_cases/input11 avg time: 1.41
test_cases/input12 avg time: 3.55
test_cases/input13 avg time: 4.25
test_cases/input15 avg time: .60
test_cases/inputa avg time: 0
test_cases/inputb avg time: 25.75
```

### Correctness Test results
```
$ ./run_test.sh 
Wed Sep 28 21:48:52 PDT 2016
test_cases/input1: PASS!  (result matched expected: 12)
test_cases/input2: PASS!  (result matched expected: 67)
test_cases/input3: PASS!  (result matched expected: 375)
test_cases/input4: PASS!  (result matched expected: 13842)
test_cases/input5: PASS!  (result matched expected: 0)
test_cases/input10: PASS!  (result matched expected: 33380)
test_cases/input11: PASS!  (result matched expected: 12705)
test_cases/input12: PASS!  (result matched expected: 63318)
test_cases/input13: PASS!  (result matched expected: 32735)
test_cases/input15: PASS!  (result matched expected: 655)
test_cases/input20: PASS!  (result matched expected: 1)
test_cases/input21: PASS!  (result matched expected: 1)
test_cases/input22: PASS!  (result matched expected: 1)
test_cases/input23: PASS!  (result matched expected: 1)
test_cases/input24: PASS!  (result matched expected: 0)
test_cases/input25: PASS!  (result matched expected: 0)
test_cases/input26: PASS!  (result matched expected: 1)
test_cases/input27: PASS!  (result matched expected: 1)
test_cases/input28: PASS!  (result matched expected: 1)
test_cases/inputa: PASS!  (result matched expected: 2)
test_cases/inputb: PASS!  (result matched expected: 301716)
```


## Disclosure
I didn't start this repo until I already had six versions of the project.  I've gone back to clean up each version and commit it, so you (and I) can see the code evolve.  And then I continued to work on the project.
