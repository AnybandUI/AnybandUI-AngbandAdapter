param([string]$Engine = 'C:/Users/developer/source/repos/angband')
$ErrorActionPreference = 'Stop'
$adapter = Split-Path (Split-Path $PSScriptRoot)
$build = "$adapter/build/vanilla-modules"
$old = "$Engine/.worktrees/direct-frontend/src"
$dump = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe'
foreach ($dir in 'src','sdk','presentation','tests') { New-Item -ItemType Directory -Force "$PSScriptRoot/$dir" | Out-Null }
Copy-Item "$adapter/src/*" "$PSScriptRoot/src"
Copy-Item "$adapter/experiments/direct-frontend/src/*" "$PSScriptRoot/src" -Force
Copy-Item "$adapter/experiments/direct-frontend/tests/*" "$PSScriptRoot/tests" -Force
Copy-Item "$Engine/src/*.h" "$PSScriptRoot/sdk"
# This frontend owns these UI implementations. No gameplay object is linked.
$exclude = @('ui-entry.c','ui-entry-combiner.c','ui-entry-renderers.c','ui-equip-cmp.c','ui-term.c','ui-visuals.c')
$ui = @(Get-ChildItem "$Engine/src/ui-*.c" | Where-Object {$_.Name -notin $exclude})
foreach ($file in $ui) {
    Copy-Item "$old/$($file.Name)" "$PSScriptRoot/presentation/$($file.Name)"
    $header = [IO.Path]::ChangeExtension($file.Name, '.h')
    if (Test-Path "$old/$header") { Copy-Item "$old/$header" "$PSScriptRoot/sdk/$header" }
}
# Term events crossing the engine boundary retain vanilla's exact layout.
$eventHeader = Get-Content "$Engine/src/ui-event.h" -Raw
$eventHeader = $eventHeader.Replace('struct mouseclick {', "#define MOUSE_MOD_GRID 0x80`nstruct mouseclick {")
$eventHeader | Set-Content "$PSScriptRoot/sdk/ui-event.h"
foreach ($name in 'obj-info.c','ui-map-presentation.c') { Copy-Item "$old/$name" "$PSScriptRoot/presentation/$name" }
foreach ($name in 'obj-info.h','z-textblock.h','ui-map.h') { Copy-Item "$old/$name" "$PSScriptRoot/sdk/$name" }
# Only presentation's private grid descriptor changes; never engine structures.
$cave = Get-Content "$old/cave.h" -Raw
$cave = $cave -replace '\bgrid_data\b','frontend_grid_data'
$cave | Set-Content "$PSScriptRoot/sdk/cave.h"
$map = Get-Content "$old/cave-map.c" -Raw
$start = $map.IndexOf('static void map_info_internal(')
$end = $map.IndexOf('/**', $map.IndexOf('void map_info_readonly(', $start))
(Get-Content "$old/cave-map.c" -Raw).Substring(0, $map.IndexOf('/**', $map.IndexOf('#include'))) | Set-Content "$PSScriptRoot/presentation/map-read.c"
$map.Substring($start, $end-$start) | Add-Content "$PSScriptRoot/presentation/map-read.c"
# Stock store UI's purchasing calculation belongs to this frontend.
$store = Get-Content "$old/store.c" -Raw
$start = $store.IndexOf('bool store_purchase_limit(')
$end = $store.IndexOf('/**', $start)
@('#include "angband.h"', '#include "store.h"', '#include "obj-gear.h"', '#include "obj-util.h"', '#include "obj-knowledge.h"', '#include "message.h"', $store.Substring($start,$end-$start)) | Set-Content "$PSScriptRoot/presentation/store-limit.c"
'bool store_purchase_limit(struct store *, const struct object *, int *, int *);' | Add-Content "$PSScriptRoot/sdk/store.h"
# Existing parser is already a public data symbol in vanilla.
'extern struct file_parser constants_parser;' | Add-Content "$PSScriptRoot/sdk/init.h"
$feel = Get-Content "$old/cmd-cave.c" -Raw
$start = $feel.IndexOf('const char *const obj_feeling_text')
$end = $feel.IndexOf('/**', $feel.IndexOf('};', $feel.IndexOf('const char *const mon_feeling_text', $start)))
$feel.Substring($start,$end-$start) | Set-Content "$PSScriptRoot/presentation/feelings.c"
@('extern const char *const obj_feeling_text[11];','extern const char *const mon_feeling_text[10];') | Add-Content "$PSScriptRoot/sdk/cmds.h"
# Describe exactly which DLL data are local versus imported, using compiler output.
$local = [Collections.Generic.HashSet[string]]::new()
foreach ($file in @($ui) + @(Get-Item "$Engine/src/obj-info.c")) {
    $obj = "$build/CMakeFiles/OurCoreLib.dir/src/$($file.Name).obj"
    & $dump /symbols $obj | ForEach-Object {
        if ($_ -match '^\w+ ([0-9A-F]+) (SECT\w+|UNDEF).*External\s+\| _(\w+)$') {
            if ($Matches[2] -ne 'UNDEF' -or $Matches[1] -ne '00000000') { [void]$local.Add($Matches[3]) }
        }
    }
}
$data = [Collections.Generic.HashSet[string]]::new()
Get-Content "$build/CMakeFiles/OurExecutable.dir/exports.def" | ForEach-Object {
    if ($_ -match '^\s+(\w+)\s+DATA$' -and !$local.Contains($Matches[1])) { [void]$data.Add($Matches[1]) }
}
foreach ($file in Get-ChildItem "$PSScriptRoot/sdk" -Filter '*.h') {
    $content = Get-Content $file.FullName -Raw
    $content = $content -replace '\bgrid_data\b','frontend_grid_data'
    $content = [regex]::Replace($content, '\bextern\s+[^;]+;', [System.Text.RegularExpressions.MatchEvaluator]{param($m)
        $decl = $m.Value
        $plain = [regex]::Replace($decl, '/\*[\s\S]*?\*/', '')
        $name = ''
        if ($plain -match '\(\s*\*\s*(\w+)\s*\)') { $name = $Matches[1] }
        elseif ($plain -notmatch '\(' -and $plain -match '(\w+)\s*(\[[^;]*\])?\s*;$') { $name = $Matches[1] }
        if ($data.Contains($name)) { return $decl.Replace('extern ', 'extern __declspec(dllimport) ') }
        return $decl
    })
    $content | Set-Content $file.FullName
}
foreach ($file in Get-ChildItem "$PSScriptRoot/presentation" -Filter '*.c') {
    (Get-Content $file.FullName -Raw) -replace '\bgrid_data\b','frontend_grid_data' | Set-Content $file.FullName
}
foreach ($file in Get-ChildItem "$PSScriptRoot/src" -Filter '*.h') {
    (Get-Content $file.FullName -Raw) -replace '\bgrid_data\b','frontend_grid_data' | Set-Content $file.FullName
}
@{Engine=(git -C $Engine rev-parse HEAD); EngineBuild=$build; LocalPresentationFiles=$ui.Count + 4; Note='Generated SDK; UI/presentation implementations are adapter-owned; no engine archive or core object linked.'} | ConvertTo-Json | Set-Content "$PSScriptRoot/provenance.json"
'Prepared frontend sources and data imports.'

foreach ($file in Get-ChildItem "$PSScriptRoot/tests" -Filter '*.h') {
    (Get-Content $file.FullName -Raw) -replace '\bgrid_data\b','frontend_grid_data' | Set-Content $file.FullName
}

