#!/bin/sh
sed 's|TEST\(.*\)); //\(.*\)|TEST\1,"\2");|'  -i testsuite.cc

