#!/bin/bash
rm -rf linuxbuild
mkdir linuxbuild
cd linuxbuild
cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_BOOST=OFF -DSOCI_EMPTY=OFF -DWITH_ORACLE=OFF -DWITH_ODBC=ON -DWITH_MYSQL=OFF -DWITH_POSTGRESQL=OFF -DWITH_FIREBIRD=OFF -DWITH_DB2=OFF -DSOCI_CXX_C11=ON -DSOCI_STATIC=ON -DSOCI_SHARED=OFF -DSOCI_TESTS=OFF ../src
cmake --build .
