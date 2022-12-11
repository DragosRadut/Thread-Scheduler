#!/bin/bash
make
cp libscheduler.so ./checker-lin/
cd checker-lin/
make -f Makefile.checker
#LD_LIBRARY_PATH=. valgrind -s --leak-check=full --show-leak-kinds=all ./_test/run_test 9
rm libscheduler.so
