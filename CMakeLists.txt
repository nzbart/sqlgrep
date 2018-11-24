cmake_minimum_required(VERSION 3.12)
project(sqlgrep)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(sqlgrep
    sqlgrep/sqlgrep.cpp
    sqlgrep/linux_specific.cpp
    ThirdParty/fmt_source/format.cc)
include_directories(ThirdParty)

# SOCI
include_directories(ThirdParty/soci/linuxbuild/include)
include_directories(ThirdParty/soci/src/include)
set(SOCI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/soci/linuxbuild/lib")
find_library(SOCI_CORE_LIBRARY NAMES libsoci_core.a PATHS ${SOCI_LIBRARY_DIR})
find_library(SOCI_ODBC_LIBRARY NAMES libsoci_odbc.a PATHS ${SOCI_LIBRARY_DIR})
target_link_libraries(sqlgrep ${SOCI_CORE_LIBRARY} ${SOCI_ODBC_LIBRARY} odbc dl)
