#!/bin/bash
./test.sh "$@" |& tee -a test_results/test_output
