# sqlgrep
Search through text columns in a database.

## Download

Download the latest version here, no installation required:
* [Download from Github](https://github.com/nzbart/sqlgrep/releases/download/v0.2/sqlgrep.exe)

Don't trust random executables from the internet? Build it yourself. You will need [Visual Studio](https://visualstudio.microsoft.com/vs/) with C++ installed. Then run:

```
git clone https://github.com/nzbart/sqlgrep.git
msbuild sqlgrep/sqlgrep.sln /p:Configuration=Release
sqlgrep/x64/Release/sqlgrep --help
```

## Usage

```
./sqlgrep --help                    # see all options
./sqlgrep haystack_database needle  # search for needle in haystack database
```
