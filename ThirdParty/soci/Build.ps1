$ErrorActionPreference = 'Stop'

pushd $PSScriptRoot
try {
    if(test-path build) { rm -r -fo build }
    md build | cd
    cmake -G "Visual Studio 15 Win64" -DWITH_BOOST=OFF -DWITH_ORACLE=OFF -DWITH_ODBC=ON -DWITH_MYSQL=OFF -DWITH_POSTGRESQL=OFF -DWITH_FIREBIRD=OFF -DWITH_DB2=OFF -DSOCI_CXX_C11=ON -DSOCI_STATIC=ON -DSOCI_SHARED=OFF -DSOCI_TESTS=OFF ..
    cmake --build . --config Debug
    cmake --build . --config Release
}
finally {
    popd
}