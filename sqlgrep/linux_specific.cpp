#include "pch.h"

auto set_up_console()
{
}

auto on_driver_not_found()
{
    fmt::print(stderr, "WARNING: No ODBC driver found. Please install it using these instructions: https://docs.microsoft.com/en-us/sql/connect/odbc/linux-mac/installing-the-microsoft-odbc-driver-for-sql-server?view=sql-server-2017\n");
}

