# sqlgrep
Search through text columns in a database.

## Download

Download the latest version here, no installation required:
* [Download from Github](https://github.com/nzbart/sqlgrep/releases/download/v0.2/sqlgrep.exe)

Don't trust me? Build it yourself. You will need Visual Studio 2017 installed and a copy of this repository on your local disk. Then run:

```
msbuild .\sqlgrep.sln /p:Configuration=Release
```

The executable will be built into `x64/Release`.

## Usage

```
./sqlgrep --help                    # see all options
./sqlgrep haystack_database needle  # search for needle in haystack database
```
