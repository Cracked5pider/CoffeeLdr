param ([Parameter(Mandatory)]$TopSearchDir)
Get-ChildItem -Path $TopSearchDir -Filter *.o -Recurse -File -Name  -Exclude *x86* | ForEach-Object { [System.IO.Path]::GetFullPath($_)} | Set-Content FileList.txt
