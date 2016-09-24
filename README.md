# ducts
This is a record of my attempts at Quora's Datacenter Cooling problem.

I originally saw the problem on Quora's careers page.
For reference, I found it again [here](http://www.businessinsider.com/heres-the-test-you-have-to-pass-to-work-at-quora-silicon-valleys-hot-new-86-million-startup-2010-4).

# Version history
Ver | Comments
---:| :----
1.0 | Initial release
1.1 | Added this block of comments
2.0 | Using a 64-bit `uint64_t` instead of an array to store the room structure. This limits `width*length` to 64. Credit for the idea to [Pathikrit Bhowmick](https://github.com/pathikrit/Quora-Challenges/). The original problem description doesn't include any size limits, but I still feel silly for not thinking of this myself.
2.1 | Performance bug fix: No longer continuing to search after reaching the end room.  But it turns out, this is still slower than version 1.  I had thought copying the whole room structure (as a single `uint64_t`) onto the stack when recursing would help avoid data hazards in the processor.  But it seems there are much bigger bottlenecks: possibly the function calls themselves, branching, and multiplication in position calculation.  Will have to give the compiler some more help!  Perhaps handling the search in a loop instead of a recursive function call would help reduce dependencies?  Also, now that the array structure is copied at every search level, the search could easily be given to several threads.
3.0 | 3.0 Replace position struct with a `uint64_t` bitmask into the room structure. Exactly one bit is set in a position bitmask at any time. Finally, something faster than version 1! But only about 25% faster.
3.1 | Simplified search2() with a (kind of complicated) macro. Performance unchanged. This let me play with macros, but it would be interesting to do the same thing with a helper function and some indirection. I wonder how much the compiler could optimize it away.



# How to run
`compile.sh` compiles with `gcc -O3 -oducts ducts.c`.

`test_cases/` directory contains test cases and expected results.  `inputa` and `inputb` are the small and big examples given in the original problem statement.  The other examples fall somewhere in between.

`run_test.sh` runs the test cases.  Test output goes to stdout as well as the `test_output` file.  You can pass a list of test cases if you don't want to run them all: `./run_test.sh test_cases/input{1,2,3}`.  The corresponding output files are automatically used to check the results.

`run_perf_test.sh` runs each test case ten times and outputs the average run time (centi-second precision).  Only counts user time, as system time is negligible (on the order of 0.01%).  Test output goes to stdout as well as the `perf_test_output` file.  You can pass a list of test cases if you don't want to run them all: `./run_perf_test.sh test_cases/input1 test_cases/input2`.  The output is ignored (not checking for correctness).


## Performance test results
Skipping inputb, since it would take all day to run ten times!
Future versions should be several orders of magnitude faster.
```
$ ./run_perf_test.sh test_cases/input{1,2,3,4,5,a}Fri Sep 23 15:42:05 PDT 2016
test_cases/input1 avg time: 0
test_cases/input2 avg time: 0
test_cases/input3 avg time: .16
test_cases/input4 avg time: 18.72
test_cases/input5 avg time: 114.33
test_cases/inputa avg time: 0
```

## Test results
```
$ ./run_test.sh
Fri Sep 23 15:31:48 PDT 2016
test_cases/input1: PASS!  (result matched expected: 12)
test_cases/input2: PASS!  (result matched expected: 67)
test_cases/input3: PASS!  (result matched expected: 375)
test_cases/input4: PASS!  (result matched expected: 13842)
test_cases/input5: PASS!  (result matched expected: 0)
test_cases/inputa: PASS!  (result matched expected: 2)
```


## Disclosure
I didn't start this repo until I already had six versions of the project.  So I've gone back and cleaned up the first version to put up.  I'm not sure if I'll take the time to clean up the rest, except the last.  I'll do enough to compare speeds, though.  My latest version finishes inputb in a few minutes.

