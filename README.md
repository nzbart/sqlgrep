[![Build Status](https://travis-ci.org/nzbart/sqlgrep.svg?branch=master)](https://travis-ci.org/nzbart/sqlgrep)
![Chocolatey](https://img.shields.io/chocolatey/v/sqlgrep.svg)
![GitHub release](https://img.shields.io/github/release/nzbart/sqlgrep.svg)
![GitHub commits (since latest release)](https://img.shields.io/github/commits-since/nzbart/sqlgrep/latest.svg)

# sqlgrep
Search through text columns in a database.

## Installation

Install via [Chocolatey](https://chocolatey.org/):

```powershell
choco install sqlgrep
```

For other options, including for Linux, see the [Download](#Download) section.

## Usage

```powershell
# search for needle in haystack database on localhost using trusted authentication
./sqlgrep haystack_database needle

# search for needle in haystack database on the specified server using trusted authentication
./sqlgrep haystack_database needle -s server

# search for needle in haystack database on localhost using username and password
./sqlgrep haystack_database needle -u user -p pass

# see all options
./sqlgrep --help
```

## Features and limitations

These limitations are currently in place, but it would be great if they could be removed:

* Only tested against SQL Server.
* Only supports a basic substring search.
* Does not output results in an easily machine parseable format, such as XML or JSON.

There are no plans to make a GUI, since there are already some nice GUI tools available with search features, such as [HeidiSQL](https://www.heidisql.com/).

## Download

Download the latest Windows version here, no installation required:
* [Download from Github](https://github.com/nzbart/sqlgrep/releases/download/v0.4/sqlgrep.exe)

Don't trust random executables from the internet? Build it yourself.

### Building on Windows
You will need [Visual Studio](https://visualstudio.microsoft.com/vs/) with C++ installed. Then run:

```powershell
git clone https://github.com/nzbart/sqlgrep.git
msbuild sqlgrep/sqlgrep.sln /p:Configuration=Release
sqlgrep/x64/Release/sqlgrep --help
```

### Building on Linux
The instructions below work for Debian 9, but can be adapted for other variants:
```sh
sudo apt install unixodbc-dev
git clone https://github.com/nzbart/sqlgrep.git
cd sqlgrep
./LinuxBuild.sh
```

