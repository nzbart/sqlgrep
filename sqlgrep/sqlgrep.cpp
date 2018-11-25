#include "pch.h"
#include "sqlgrep.h"

using namespace std;
using namespace soci;

struct column_details
{
    string schema;
    string table;
    string column;
    uint64_t number_of_rows;
};

struct match_details
{
    column_details column;
    bool more_matches_available;
    vector<string> matches;
};

bool verbose_messages_enabled = false;

string const clear_eol = "\033[0K"s;

auto write_colour(string_view const message, fmt::color colour)
{
    fmt::print(colour, "{}{}\n", clear_eol, message);
}

auto write_verbose(string_view const message)
{
    if (verbose_messages_enabled)
    {
        write_colour(message, fmt::color::green);
    }
}

auto write_error(string_view const message)
{
    fmt::print(fmt::color::red, "{}\n", message);
}

class database_query_logger : public logger_impl
{
public:
    void start_query(std::string const & query) override
    {
        write_verbose("Query: "s + query);
    }

private:
    logger_impl* do_clone() const override
    {
        return new database_query_logger;   // `new` is required by SOCI
    }
};

auto write_progress(uint64_t total, uint64_t completed, chrono::duration<uint64_t, std::nano> time_taken_so_far)
{
    uint64_t const nanoseconds_per_second = 1'000'000'000L;
    uint64_t const seconds_so_far = time_taken_so_far.count() / nanoseconds_per_second;
    uint64_t const estimated_total_seconds = seconds_so_far * total / completed;
    uint64_t const remaining_seconds = estimated_total_seconds - seconds_so_far;
    auto const percent_complete = completed * 100 / total;
    fmt::print("{}{}% complete", clear_eol, percent_complete);
    if (percent_complete > 10)
        fmt::print(" ({} seconds remaining)...", remaining_seconds);
    fmt::print("\r");
}

auto enquote(string_view const val)
{
    return "\""s + string(val) + "\"";
}

auto get_number_of_rows(session & sql, string_view const schema, string_view const table, string_view const column, unordered_map<string, uint64_t> & cache)
{
    uint64_t count;
    stringstream query;
    query << "select count(*) from " << enquote(schema) << "." << enquote(table);
    auto const query_str = query.str();

    auto const cached = cache.find(query_str);
    if (cached != cache.end())
        return cached->second;

    sql << query_str, into(count);

    cache[query_str] = count;

    return count;
}

auto get_all_string_columns(session & sql)
{
    fmt::print("Scanning for string columns...\n");

    rowset<row> data = sql.prepare <<
        "select TABLE_SCHEMA SchemaName, TABLE_NAME TableName, COLUMN_NAME ColumnName "
        "from INFORMATION_SCHEMA.COLUMNS c "
        "where DATA_TYPE in('char', 'varchar', 'nchar', 'nvarchar') and "
        "(select TABLE_TYPE from INFORMATION_SCHEMA.TABLES t where t.TABLE_SCHEMA = c.TABLE_SCHEMA and t.TABLE_NAME = c.TABLE_NAME) = 'BASE TABLE' "
        "order by TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME";

    vector<column_details> details;
    for (auto& r : data)
    {
        column_details column{ r.get<string>(0), r.get<string>(1), r.get<string>(2), 0 };
        details.push_back(column);
    }

    unordered_map<string, uint64_t> cache;
    for (auto& column : details)
    {
        column.number_of_rows = get_number_of_rows(sql, column.schema, column.table, column.column, cache);
        write_verbose("Number of rows in "s + column.schema + "." + column.table + "." + column.column + ": " + to_string(column.number_of_rows) + ".");
    }

    return details;
}

auto escape_search_text(string_view to_find)
{
    regex const r("([\\\\%_[])");
    string result;
    regex_replace(back_inserter(result), begin(to_find), end(to_find), r, "\\$1");
    return result;
}

auto find_matches(session & sql, string_view const schema, string_view const table, string_view const column, string_view const to_find, int const maximum_results_per_column)
{
    int const max_string = 500;
    vector<string> matches(maximum_results_per_column + 1);
    stringstream query;
    query << "select cast(left(" << enquote(column) << ", " << (max_string + 1) << ") as varchar(" << (max_string + 1) << ")) from " << enquote(schema) << "." << enquote(table) << " where " << enquote(column) << " like '%" + escape_search_text(to_find) + "%' escape '\\'", into(matches);
    sql << query.str(), into(matches);
    for (vector<string>::size_type i = 0; i != matches.size(); ++i)
    {
        if (matches[i].length() > max_string)
            matches[i] = matches[i].substr(0, max_string) + "... <truncated>";
    }
    return matches;
}

auto display_all_matches(session & sql, vector<column_details> const & all_columns, string_view to_find, int const maximum_results_per_column, uint64_t total_rows)
{
    uint64_t completed_rows = 0;
    auto const start_time = chrono::steady_clock::now();
    auto last_displayed = start_time;
    for (auto column : all_columns)
    {
        auto matches = find_matches(sql, column.schema, column.table, column.column, to_find, maximum_results_per_column);
        if (!matches.empty())
        {
            auto const more_available = matches.size() > maximum_results_per_column;
            if (more_available)
                matches.resize(maximum_results_per_column);
            match_details match{ column, more_available, matches };

            write_colour(match.column.table + "." + match.column.column, fmt::color::magenta);
            for (auto const & value : match.matches)
            {
                fmt::print("    {}\n", value);
            }

            if (match.more_matches_available)
                write_colour("    ... more available", fmt::color::green);
        }

        completed_rows += column.number_of_rows;
        auto const now = chrono::steady_clock::now();
        auto const nanoseconds_per_second = 1'000'000'000;
        write_verbose("Completed "s + to_string(completed_rows) + " rows in " + to_string((now - start_time).count() / nanoseconds_per_second) + " seconds.");
        if (!matches.empty() || (now - last_displayed).count() / nanoseconds_per_second > 2) {
            write_progress(total_rows, completed_rows, now - start_time);
            last_displayed = now;
        }
    }
}

auto find_and_display_matches(string_view to_find, int const maximum_results_per_column, string_view const connection_string)
{
    connection_parameters parameters(odbc, string(connection_string));
    parameters.set_option(odbc_option_driver_complete, to_string(SQL_DRIVER_NOPROMPT));
    session sql(parameters);
    sql.set_logger(new database_query_logger);  // `new` is required by SOCI
    auto all_columns = get_all_string_columns(sql);
    cout << "Searching " << all_columns.size() << " columns for '" << to_find << "'...\n";
    uint64_t const total_rows = accumulate(begin(all_columns), end(all_columns), 0, [](uint64_t acc, column_details const & b) { return acc + b.number_of_rows; });
    write_verbose("Total number of rows to search: "s + to_string(total_rows) + ".");
    display_all_matches(sql, all_columns, to_find, maximum_results_per_column, total_rows);
}

auto get_all_odbc_drivers()
{
    SQLHANDLE environment_handle;
    auto ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &environment_handle);
    if (is_odbc_error(ret))
        throw runtime_error("Unable to get environment handle.");

    ret = SQLSetEnvAttr(environment_handle, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void*>(SQL_OV_ODBC3), 0);
    if (is_odbc_error(ret))
        throw runtime_error("Unable to configure environment to ODBC version 3.");

    vector<string> drivers;
    int const buffer_size = 2000;
    unsigned char description[buffer_size];
    unsigned char driver_attributes[buffer_size];
    while (true) {
        short actual_description_length;
        short actual_driver_attributes_length;
        ret = SQLDriversA(
            environment_handle,
            SQL_FETCH_NEXT,
            description,
            buffer_size,
            &actual_description_length,
            driver_attributes,
            buffer_size,
            &actual_driver_attributes_length);
        switch(ret)
        {
        case SQL_SUCCESS:
            drivers.emplace_back(string(reinterpret_cast<char*>(description), actual_description_length));
            break;
        case SQL_SUCCESS_WITH_INFO:
            throw runtime_error("The buffer was not long enough for the driver description or attributes.");
        case SQL_NO_DATA:
            return drivers;
        case SQL_ERROR:
            throw soci_error("Unable to enumerate ODBC drivers");
        default:
            throw runtime_error("Unexpected result code from SQLDrivers: "s + to_string(ret) + ".");
        }
    }
}

auto get_newest_matching_driver(vector<string> const & all_drivers, regex const & driver_regex)
{
    smatch match;
    optional<pair<string, int>> found_driver;
    for (auto& driver : all_drivers)
    {
        if (regex_match(driver, match, driver_regex))
        {
            auto const driver_version = atoi(match[1].str().c_str());
            if(!found_driver.has_value() || found_driver.value().second < driver_version)
            {
                found_driver = make_pair(driver, driver_version);
            }
        }
    }
    return found_driver.has_value() ? make_optional(found_driver.value().first) : optional<string>();
}

auto get_best_sql_server_odbc_driver_name()
{
    auto const all_drivers = get_all_odbc_drivers();
    auto modern_odbc_driver_name = get_newest_matching_driver(all_drivers, regex(R"regex(^ODBC Driver (\d+(?:\.\d+|)) for SQL Server$)regex"));
    auto native_driver_name = get_newest_matching_driver(all_drivers, regex(R"regex(^SQL Server Native Client (\d+(?:\.\d+|))$)regex"));
    auto sql_server_driver_name = get_newest_matching_driver(all_drivers, regex(R"regex(^SQL Server$)regex"));

    if(!modern_odbc_driver_name.has_value() && !native_driver_name.has_value() && !sql_server_driver_name.has_value()) {
        on_driver_not_found();
    }

    return modern_odbc_driver_name.value_or(native_driver_name.value_or("SQL Server"));
}

int main(int argc, char** argv)
{
    set_up_console();

    CLI::App app{ "sqlgrep" };
    shared_ptr<CLI::Formatter> formatter{ new CLI::Formatter() };
    formatter->column_width(50);
    app.formatter(formatter);
    string database;
    app.add_option("database", database, "The database to search")->mandatory();
    string search_string;
    app.add_option("search-string", search_string, "The text to search for")->mandatory();
    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Verbose mode");
    int maximum_results_per_column = 5;
    app.add_option("-m,--max-results", maximum_results_per_column, "Maxmium number of matches to return per column", true);
    string server = "localhost";
    app.add_option("-s,--server", server, "The SQL server that has the database to search", true);
    optional<string> username;
    auto user_name_option = app.add_option("-u,--user-name", username, "The name of the user to connect as");
    optional<string> password;
    auto password_option = app.add_option("-p,--password", password, "The password of the user to connect as");
    user_name_option->needs(password_option);
    password_option->needs(user_name_option);
    string driver = get_best_sql_server_odbc_driver_name();
    app.add_option("-d,--driver", driver, "The ODBC database driver to use", true);

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return app.exit(e);
    }

    auto const credential = password.has_value()
        ? "Uid=" + username.value() + ";Pwd=" + password.value()
        : "Trusted_Connection=Yes";
    auto const connection_string = "Driver={" + driver + "};Server=" + server + ";Database=" + database + ";" + credential;
    verbose_messages_enabled = verbose;

    try
    {
        find_and_display_matches(search_string, maximum_results_per_column, connection_string);
        cout << clear_eol << "\n";
        return 0;
    }
    catch (odbc_soci_error & e)
    {
        if(string(reinterpret_cast<char const*>(e.odbc_error_code())) == "28000")
        {
            if (e.native_error_code() == 18452)
            {
                write_error("You attempted to connect with a trusted connection, but the current user is not trusted. Try a username and password.");
                return 2;
            }
            if(e.native_error_code() == 18456)
            {
                write_error("The login credentials may be incorrect.");
                return 2;
            }
        }

        write_error("DB error: "s + e.what());
        return 1;
    }
    catch (soci_error& e)
    {
        write_error("DB error: "s + e.what());
        return 1;
    }
    catch (exception& e)
    {
        write_error("Generic error: "s + e.what());
        return 1;
    }
    catch (...)
    {
        write_error("Something went wrong.");
        return 3;
    }
}
