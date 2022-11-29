#!/bin/bash
make
cp libscheduler.so ./checker-lin/
cd checker-lin/
make -f Makefile.checker
#LD_LIBRARY_PATH=. ./_test/run_test 7
rm libscheduler.so
