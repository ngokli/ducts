# ducts
This is a record of my attempts at Quora's Datacenter Cooling problem.

I originally saw the problem on Quora's careers page.
For reference, I found it again [here](http://www.businessinsider.com/heres-the-test-you-have-to-pass-to-work-at-quora-silicon-valleys-hot-new-86-million-startup-2010-4).

`test_cases/` directory contains test cases and expected results.  `inputa` and `inputb` are the small and big examples given in the original problem statement.  The other examples fall somewhere in between.

`run_test.sh` runs the test cases.  Test output goes to stdout as well as the `test_output` file.  You can pass a list of test cases if you don't want to run them all: `./run_test.sh test_cases/input{1,2,3}`.  The corresponding output files are automatically used to check the results.

`run_perf_test.sh` runs each test case ten times and outputs the average run time (centi-second precision).  Only counts user time, as system time is negligible.  Test output goes to stdout as well as the `perf_test_output` file.  You can pass a list of test cases if you don't want to run them all: `./run_perf_test.sh test_cases/input1 test_cases/input2`.  The output is ignored (not checking for correctness).

I compile with `gcc -O3 -oducts ducts.c`. (in `compile.sh`)

## Performance test results:
```
$ ./run_perf_test.sh test_cases/input{a,1,2,3,4,5}
Wed Sep 21 22:44:58 PDT 2016
test_cases/inputa avg time: 0
test_cases/input1 avg time: 0
test_cases/input2 avg time: 0
test_cases/input3 avg time: .23
test_cases/input4 avg time: 25.36
test_cases/input5 avg time: 149.32
```

Manually ran just once for inputb, since it takes so long.  Note the system time used.
```
$ /usr/bin/time -v ./ducts -q < test_cases/inputb 
  Command being timed: "./ducts"
  User time (seconds): 4689.57
  System time (seconds): 0.70
```

## Test results
```
$ ./run_test.sh
Wed Sep 21 22:42:06 PDT 2016
test_cases/input1: PASS!  (result matched expected: 12)
test_cases/input2: PASS!  (result matched expected: 67)
test_cases/input3: PASS!  (result matched expected: 375)
test_cases/input4: PASS!  (result matched expected: 13842)
test_cases/input5: PASS!  (result matched expected: 0)
test_cases/inputa: PASS!  (result matched expected: 2)
test_cases/inputb: PASS!  (result matched expected: 301716)
```


## Disclosure
I didn't start this repo until I already had six versions of the project.  So I've gone back and cleaned up the first version to put up.  I'm not sure if I'll take the time to clean up the rest, except the last.  I'll do enough to compare speeds, though.  My latest version finishes inputb in a few minutes.
