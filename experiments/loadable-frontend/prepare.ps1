$ErrorActionPreference = 'Stop'
$probe = $PSScriptRoot
$adapter = 'C:\Users\developer\source\repos\AnybandUI-AngbandAdapter'
$engine = 'C:\Users\developer\source\repos\angband\.worktrees\direct-frontend'
$build = "$adapter\build\direct-frontend"
$dump = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe'
$imports = @{}
$obj = (Get-ChildItem $build -Recurse -Filter '*main-anybandui.c.obj').FullName
& $dump /symbols $obj | ForEach-Object {
    if ($_ -match 'UNDEF.*External\s+\| (\w+)$') {
        $name = $Matches[1]
        $imports[$name] = $_.Contains('notype ()')
    }
}
$definitions = [System.Collections.Generic.HashSet[string]]::new()
& $dump /symbols "$build\anybandui_engine.lib" | ForEach-Object {
    if ($_ -match '^\w+ ([0-9A-F]+) (SECT\w+|UNDEF).*External\s+\| (\w+)$') {
        if ($Matches[2] -ne 'UNDEF' -or $Matches[1] -ne '00000000') {
            [void]$definitions.Add($Matches[3])
        }
    }
}
$symbols = @($imports.Keys | Where-Object {$definitions.Contains($_)} | Sort-Object)
$symbols | Set-Content "$probe\symbols.txt"
[pscustomobject]@{Functions=@($symbols | Where-Object {$imports[$_]}).Count; Data=@($symbols | Where-Object {!$imports[$_]}).Count; Total=$symbols.Count} | ConvertTo-Json | Set-Content "$probe\inventory.json"
New-Item -ItemType Directory -Force "$probe\adapter-src" | Out-Null
Copy-Item "$adapter\src\*" "$probe\adapter-src"
Copy-Item "$adapter\experiments\direct-frontend\src\*" "$probe\adapter-src" -Force
$headers = [System.Collections.Generic.List[string]]::new()
$main = Get-Content "$probe\adapter-src\main-anybandui.c"
$files = @(Get-Item "$probe\adapter-src\main-anybandui.c") + @(Get-ChildItem "$probe\adapter-src" -Filter '*.h')
foreach ($file in $files) {
    $lines = foreach ($line in (Get-Content $file.FullName)) {
        if ($line -match '^#include "([^"]+)"' -and (Test-Path "$engine\src\$($Matches[1])")) {
            $header = $Matches[1]
            if (!$headers.Contains($header)) { $headers.Add($header) }
        } else { $line }
    }
    $content = $lines -join "`n"
    $content = $content.Replace('seen, hallucinated, player;', 'seen, hallucinated, is_player;')
    $content = $content.Replace('visual->player', 'visual->is_player').Replace('v->player', 'v->is_player')
    if ($file.Name -eq 'anybandui-tuning.h') { $content = $content -replace '\bmsg\b', 'message_token' }
    if ($file.Name -eq 'anybandui-projectiles.h') { $content = $content -replace '\bdistance\b', 'tile_distance' }
    if ($file.Name -eq 'anybandui-compare.h') {
        $split = $content.IndexOf('static void comparison_change')
        $content = ($content.Substring(0, $split) -replace '\bbrands\b', 'brand_damage' -replace '\bslays\b', 'slay_damage') + $content.Substring($split)
    }
    # Rebind adapter-owned C identifiers only; never transform engine source.
    # Strings/comments, member names and type tags retain their original meaning.
    $tokens = [regex]::Matches($content, '(?m)^\s*#.*$|/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*''|\w+|->|\s+|[^\w\s]')
    $rewritten = [System.Text.StringBuilder]::new()
    $previous = ''
    foreach ($token in $tokens) {
        $value = $token.Value
        if ($value -match '^\s*$' -or $value.StartsWith('/*') -or $value.StartsWith('//') -or $value.TrimStart().StartsWith('#')) {
            [void]$rewritten.Append($value)
            continue
        }
        if ($symbols -ccontains $value -and $previous -notin @('.', '->', 'struct', 'union', 'enum')) {
            [void]$rewritten.Append('(*frontend_engine_ptr->p_' + $value + ')')
        } else { [void]$rewritten.Append($value) }
        $previous = $value
    }
    $content = $rewritten.ToString()
    $content | Set-Content $file.FullName
}
@('#ifndef FRONTEND_ENGINE_API_H', '#define FRONTEND_ENGINE_API_H') + @($headers | ForEach-Object { '#include "' + $_ + '"' }) + @('struct frontend_engine_api {') + @($symbols | ForEach-Object {'    typeof(&' + $_ + ') p_' + $_ + ';'}) + @('};', '#endif') | Set-Content "$probe\engine-api.h"
@('#include "engine-api.h"', 'const struct frontend_engine_api frontend_engine = {') + @($symbols | ForEach-Object {'    &' + $_ + ','}) + @('};') | Set-Content "$probe\engine-api.c"
@($symbols | ForEach-Object {'FRONTEND_SYMBOL(' + $_ + ')'}) | Set-Content "$probe\engine-symbols.inc"
Get-Content "$probe\inventory.json"
