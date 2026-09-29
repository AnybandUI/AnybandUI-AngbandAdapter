$ErrorActionPreference = 'Stop'
$adapter = Split-Path (Split-Path $PSScriptRoot)
$dump = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe'
$local = [Collections.Generic.HashSet[string]]::new()
Get-ChildItem "$adapter/build/vanilla-adapter/CMakeFiles/anybandui.dir" -Recurse -Filter '*.obj' | ForEach-Object {
    & $dump /symbols $_.FullName | ForEach-Object {
        if ($_ -match '^\w+ [0-9A-F]+ SECT\w+.*External\s+\| _(\w+)$') { [void]$local.Add($Matches[1]) }
    }
}
$functions = [Collections.Generic.HashSet[string]]::new()
Get-Content "$adapter/build/vanilla-modules/CMakeFiles/OurExecutable.dir/exports.def" | ForEach-Object {
    if ($_ -match '^\s+(\w+)\s*$' -and !$local.Contains($Matches[1])) { [void]$functions.Add($Matches[1]) }
}
foreach ($file in Get-ChildItem "$PSScriptRoot/sdk" -Filter '*.h') {
    $content = Get-Content $file.FullName -Raw
    $content = [regex]::Replace($content, '(?m)^([A-Za-z_][\w\s*]*?\b)(\w+)(\s*\([^;{}]*\)\s*;)', [Text.RegularExpressions.MatchEvaluator]{param($m)
        if ($functions.Contains($m.Groups[2].Value) -and $m.Groups[1].Value -notmatch '\b(static|typedef|dllimport)\b') { return '__declspec(dllimport) ' + $m.Value }
        return $m.Value
    })
    $content | Set-Content $file.FullName
}
