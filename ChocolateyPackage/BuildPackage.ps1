$ErrorActionPreference = 'Stop'

pushd $PSScriptRoot
try {
    $readme = Invoke-WebRequest -UseBasicParsing https://raw.githubusercontent.com/nzbart/sqlgrep/master/README.md | select -expand Content
    if($readme -notmatch '\[Download from Github\]\((https://github.com/nzbart/sqlgrep/releases/download/v[^/]+/sqlgrep\.exe)\)') {
        throw "Couldn't find URL to download latest version from Github readme."
    }
    $downloadUrl = $Matches[1]

    if(Test-Path dist) {
        rm -r -fo dist
    }
    md dist | cd
    cp -r ../sqlgrep
    cd sqlgrep
    Invoke-WebRequest -UseBasicParsing -Uri $downloadUrl -OutFile "tools\sqlgrep.exe"
    choco pack
    mv sqlgrep.*.nupkg ..
}
finally
{
    popd
}