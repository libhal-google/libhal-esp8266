#!/bin/bash

FAILURE_COUNT=0

for test in "$@"
do
  echo
  echo "============= $test ============= "
  ./$test
  if [[ $? -ne 0 ]] ; then
    FAILURE_COUNT=$(($FAILURE_COUNT + 1))
  fi
done

echo "Number of test failures = $FAILURE_COUNT"

exit $FAILURE_COUNT
