#!/bin/bash

DUCTS_EXE=./ducts

echo "`date`"

if [ $# -eq 0 ]
then
  test_files="$(ls test_cases/input* | sort)"
else
  test_files="$@"
fi

error=0

if [[ ! -f $DUCTS_EXE ]]
then
  echo "$DUCTS_EXE does not exist."
  error=1
fi

for test_file in $test_files
do
  if [[ ! -f $test_file ]]
  then
    echo "$test_file does not exist."
    error=2
  fi
  result_file=${test_file/input/output}
  if [[ ! -f $result_file ]]
  then
    echo "$result_file does not exist."
    error=3
  fi
done

if [[ 0 -ne $error ]]
then
  exit $error
fi


for test_file in $test_files
do
  result_file=${test_file/input/output}
  expected_result="$(cat $result_file)"

  test_result="$(./ducts < $test_file)"

  if [[ $test_result -ne $expected_result ]]
  then
    echo "$test_file: FAIL!  (expected: $expected_result, got: $test_result)"
  else
    echo "$test_file: PASS!  (result matched expected: $expected_result)"
  fi
done

