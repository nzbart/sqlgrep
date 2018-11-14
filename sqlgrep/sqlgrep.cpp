#include "pch.h"

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

std::ostream& clear_eol(std::ostream& stream)
{
    stream << "\033[0K";
    return stream;
}

void write_colour(string_view const message, std::ostream& colour(std::ostream& stream), std::ostream& background(std::ostream& stream), bool is_bold = false)
{
    cout << clear_eol << colour << background << (is_bold ? termcolor::bold : colour) << message << termcolor::reset << endl;
}

void write_colour(string_view const message, std::ostream& colour(std::ostream& stream))
{
    cout << clear_eol << colour << message << termcolor::reset << endl;
}

void write_verbose(string_view const message)
{
    if (verbose_messages_enabled)
    {
        write_colour(message, termcolor::green);
    }
}

void write_error(string_view const message)
{
    write_colour(message, termcolor::red);
}

void write_progress(uint64_t total, uint64_t completed, chrono::duration<uint64_t, std::nano> time_taken_so_far)
{
    uint64_t const nanoseconds_per_second = 1'000'000'000L;
    uint64_t const seconds_so_far = time_taken_so_far.count() / nanoseconds_per_second;
    uint64_t const estimated_total_seconds = seconds_so_far * total / completed;
    uint64_t const remaining_seconds = estimated_total_seconds - seconds_so_far;
    auto const percent_complete = completed * 100 / total;
    cout << clear_eol << percent_complete << "% complete";
    if (percent_complete > 10)
        cout << " (" << remaining_seconds << " seconds remaining)...";
    cout << "\r";
}

string enquote(string_view const val)
{
    return string("\"") + string(val) + "\"";
}

uint64_t get_number_of_rows(session & sql, string_view const schema, string_view const table, string_view const column, unordered_map<string, uint64_t> & cache)
{
    int count;
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

vector<column_details> get_all_string_columns(session & sql)
{
    cout << "Scanning for string columns..." << endl;

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
    }

    return details;
}

string escape_search_text(string_view to_find)
{
    regex const r("([\\\\%_[])");
    string result;
    regex_replace(back_inserter(result), begin(to_find), end(to_find), r, "\\$1");
    return result;
}

vector<string> find_matches(session & sql, string_view const schema, string_view const table, string_view const column, string_view const to_find, int const maximum_results_per_column)
{
    int const max_string = 500;
    vector<string> matches(maximum_results_per_column + 1);
    stringstream query;
    query << "select cast(left(" << enquote(column) << ", " << (max_string + 1) << ") as varchar(" << (max_string + 1) << ")) from " << enquote(schema) << "." << enquote(table) << " where " << enquote(column) << " like '%" + escape_search_text(to_find) + "%' escape '\\'", into(matches);
    write_verbose("Executing: " + query.str());
    sql << query.str(), into(matches);
    for (vector<string>::size_type i = 0; i != matches.size(); ++i)
    {
        if (matches[i].length() > max_string)
            matches[i] = matches[i].substr(0, max_string) + "... <truncated>";
    }
    return matches;
}

void display_all_matches(session & sql, vector<column_details> const & all_columns, string_view to_find, int const maximum_results_per_column, uint64_t total_rows)
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

            write_colour(match.column.table + "." + match.column.column, termcolor::red, termcolor::on_white, true);
            for (auto const & value : match.matches)
            {
                cout << "    " << value << endl;
            }

            if (match.more_matches_available)
                write_colour("    ... more available", termcolor::green);
        }

        completed_rows += column.number_of_rows;
        auto const now = chrono::steady_clock::now();
        if (!matches.empty() || (now - last_displayed).count() / 1'000'000'000 > 2) {
            write_progress(total_rows, completed_rows, now - start_time);
            last_displayed = now;
        }
    }
}

void find_and_display_matches(string_view to_find, int const maximum_results_per_column, string_view const connection_string)
{
    session sql(odbc, string(connection_string));
    auto all_columns = get_all_string_columns(sql);
    cout << "Searching " << all_columns.size() << " columns for '" << to_find << "'..." << endl;
    uint64_t const total_rows = accumulate(begin(all_columns), end(all_columns), 0, [](int acc, column_details const & b) { return acc + b.number_of_rows; });
    display_all_matches(sql, all_columns, to_find, maximum_results_per_column, total_rows);
}

void configure_console_for_ansi_escape_sequences()
{
    auto const console_window = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD startup_console_mode;
    GetConsoleMode(console_window, &startup_console_mode);
    SetConsoleMode(console_window, startup_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

int main(int argc, char** argv)
{
    configure_console_for_ansi_escape_sequences();

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
    string driver = "ODBC Driver 11 for SQL Server";
    app.add_option("-d,--driver", driver, "The ODBC database driver to use", true);

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return app.exit(e);
    }

    string const connection_string = "Driver={" + driver + "};Server=" + server + ";Database=" + database + ";Trusted_Connection=Yes";
    verbose_messages_enabled = verbose;

    try
    {
        find_and_display_matches(search_string, maximum_results_per_column, connection_string);
        cout << clear_eol << endl;
        return 0;
    }
    catch (soci_error& e)
    {
        write_error(string("DB error: ") + e.what());
        return 1;
    }
    catch (exception& e)
    {
        write_error(string("Generic error: ") + e.what());
        return 1;
    }
    catch (...)
    {
        write_error("Something went wrong.");
        return 2;
    }
}
