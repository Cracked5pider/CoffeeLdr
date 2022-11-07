param ([Parameter(Mandatory)]$TopSearchDir)
Get-ChildItem -Path $TopSearchDir -Filter *.o -Recurse -File -Name  -Exclude *x86* | ForEach-Object { [System.IO.Path]::GetFullPath($_)} | Set-Content FileList.txt

ForEach ($item in (Get-Content $pwd\FileList.txt)) {
    Start-Process -FilePath "$pwd\CoffeeLdr.x64.exe" -Wait -NoNewWindow -WorkingDirectory $pwd -ArgumentList "go",$item
    #Start-Process -FilePath "$pwd\CoffeeLdr.x64.exe" -Wait -NoNewWindow -WorkingDirectory $pwd -ArgumentList "go",$item | Set-Content Results.txt
}
Remove-Item FileList.txt
