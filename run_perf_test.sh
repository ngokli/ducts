#!/bin/bash
./perf_test.sh "$@" |& tee -a test_results/perf_test_output
