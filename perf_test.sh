#!/bin/bash

RUN_COUNT=10
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
done

if [[ 0 -ne $error ]]
then
  exit $error
fi


for test_file in $test_files
do
  total_time=0
  loop_count=0
  while [ "$loop_count" -lt "$RUN_COUNT" ]
  do
    run_time=$(/usr/bin/time --format='%U' ./ducts -q < $test_file 2>&1)
    total_time=$(echo "$run_time + $total_time" | bc)
    let "loop_count += 1"
  done
  
  avg_time=$(echo "scale=2; $total_time / ${RUN_COUNT}.0" | bc)
  echo "${test_file} avg time: ${avg_time}"
done
