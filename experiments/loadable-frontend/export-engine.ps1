param([Parameter(Mandatory)][string]$Library,
      [Parameter(Mandatory)][string]$Output,
      [Parameter(Mandatory)][string]$Dumpbin,
      [switch]$X86)
$ErrorActionPreference = 'Stop'
$exports = [System.Collections.Generic.HashSet[string]]::new()
& $Dumpbin /symbols $Library | ForEach-Object {
    if ($_ -match '^\w+ ([0-9A-F]+) (SECT\w+|UNDEF).*External\s+\| (\w+)$') {
        $value = $Matches[1]; $section = $Matches[2]; $name = $Matches[3]
        if ($section -eq 'UNDEF' -and $value -eq '00000000') { return }
        if ($X86 -and $name.StartsWith('_')) { $name = $name.Substring(1) }
        if ($name.StartsWith('_')) { return }
        [void]$exports.Add($name)
    }
}
if ($LASTEXITCODE -or !$exports.Count) { throw 'Could not enumerate engine exports' }
@('EXPORTS') + @($exports | Sort-Object) | Set-Content -LiteralPath $Output
Write-Output "$($exports.Count) named engine exports"
