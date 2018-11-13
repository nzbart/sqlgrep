$ErrorActionPreference = 'Stop'

function SetReleaseModeToStaticLinkRuntimeLibrary([string][parameter(mandatory)]$Path) {
    $projectPath = (Resolve-Path -LiteralPath $Path).ProviderPath
    Write-Host "$path -> $projectPath"
    [xml]$project = cat -LiteralPath $projectPath
    ($project.Project.ItemDefinitionGroup | ? Condition -eq "'`$(Configuration)|`$(Platform)'=='Release|x64'" ).ClCompile.RuntimeLibrary = 'MultiThreaded'
    $project.Save($projectPath)
}

pushd $PSScriptRoot
try {
    if(test-path build) { rm -r -fo build }
    md build | cd
    cmake -G "Visual Studio 15 Win64" -DWITH_BOOST=OFF -DWITH_ORACLE=OFF -DWITH_ODBC=ON -DWITH_MYSQL=OFF -DWITH_POSTGRESQL=OFF -DWITH_FIREBIRD=OFF -DWITH_DB2=OFF -DSOCI_CXX_C11=ON -DSOCI_STATIC=ON -DSOCI_SHARED=OFF -DSOCI_TESTS=OFF ../src
    SetReleaseModeToStaticLinkRuntimeLibrary src/core/soci_core_static.vcxproj
    SetReleaseModeToStaticLinkRuntimeLibrary src/backends/odbc/soci_odbc_static.vcxproj
    cmake --build . --config Debug
    cmake --build . --config Release
}
finally {
    popd
}
