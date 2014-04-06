#!/bin/bash
LLVM_DIR=~/Projects/build-llvm-cling  #the location of your llvm dir
$LLVM_DIR/Debug+Asserts/bin/jit-testing test.c --
