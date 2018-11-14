# sqlgrep
Search through text columns in a database.

## Download

Download the latest version here, no installation required:
* [Download from Github](https://github.com/nzbart/sqlgrep/releases/download/v0.4/sqlgrep.exe)

Don't trust random executables from the internet? Build it yourself. You will need [Visual Studio](https://visualstudio.microsoft.com/vs/) with C++ installed. Then run:

```powershell
git clone https://github.com/nzbart/sqlgrep.git
msbuild sqlgrep/sqlgrep.sln /p:Configuration=Release
sqlgrep/x64/Release/sqlgrep --help
```

## Usage

```powershell
# see all options
./sqlgrep --help

# search for needle in haystack database on localhost using trusted authentication
./sqlgrep haystack_database needle

# search for needle in haystack database on the specified server using trusted authentication
./sqlgrep haystack_database needle -s server

# search for needle in haystack database on localhost using username and password
./sqlgrep haystack_database needle -u user -p pass
```

## Features and limitations

These limitations are currently in place, but it would be great if they could be removed:

* Only tested against SQL Server.
* Only compiles on Windows.
* Only supports a basic substring search.
* Does not output results in an easily machine parseable format, such as XML or JSON.

There are no plans to make a GUI, since there are already some nice GUI tools available with search features, such as [HeidiSQL](https://www.heidisql.com/).
