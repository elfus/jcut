#!/bin/bash
# This script has to be executed in the project folder <project-root>/tests
#
# It will iterate over all the folders in the tests folder assuming there is
# always a C file named 'test_group.c' and a test file named test-file.txt
#
JIT_TESTING="$HOME/projects/build-release34/Release+Asserts/bin/jit-testing"

DIRECTORIES=`ls -f | grep group`
FAILED=0

echo "Current dir: `pwd`"
echo "JIT_TESTING: $JIT_TESTING"

for dir in $DIRECTORIES
do
    echo "Running tests in folder $dir"
    cd $dir
    if [ "$dir" == "groupH" ]
    then
	$JIT_TESTING test_group.c -t test-file.txt --recover 2> stderr.txt 1> stdoutput.txt 
    else
	$JIT_TESTING test_group.c -t test-file.txt 2> stderr.txt 1> stdoutput.txt 
    fi

    echo "Return code $?"
    FAILED=$((FAILED+$?))
    cd ..
done

echo "Number of failed tests: $FAILED"
