#ifndef PCH_H
#define PCH_H

#include <chrono>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>

#ifdef _WIN64
#include <Windows.h>
#endif

#include <sqlext.h>
#include "CLI11/CLI11.hpp"
#include "fmt/format.h"
#include "fmt/color.h"
#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"

#endif //PCH_H
