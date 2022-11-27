#!/bin/bash
make
cp libscheduler.so ./checker-lin/
cd checker-lin/
make -f Makefile.checker
rm libscheduler.so
