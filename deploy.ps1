$mode = $args[0]
$project_name = "bioimread"
$target_dir = "$(Get-Location)\build"
$qt_bin_dir = "D:\vcpkg\installed\x64-windows\tools\Qt6\bin"
$qt_trans_dir = "D:\vcpkg\installed\x64-windows\translations"
$qt_res_dir = "D:\vcpkg\installed\x64-windows\resources"
$release_bin_dir = "D:\vcpkg\installed\x64-windows\bin"
$debug_bin_dir = "D:\vcpkg\installed\x64-windows\debug\bin"
$copy_from_bin = $debug_bin_dir
$copy_to_bin = "$target_dir\Debug"

if ($mode -eq "release") {
    & "$qt_bin_dir\windeployqt.exe" "$target_dir\Release\$project_name.exe"
    $copy_from_bin = $release_bin_dir
    $copy_to_bin = "$target_dir\Release"
}
else {
    & "$qt_bin_dir\windeployqt.debug.bat" "$target_dir\Debug\$project_name.exe"
}

# to support jpeg
Copy-Item "$copy_from_bin\jpeg62.dll" -Destination $copy_to_bin

# to support jp2
Copy-Item "$copy_from_bin\jasper*.dll" -Destination $copy_to_bin
Copy-Item "$copy_from_bin\openjp2.dll" -Destination $copy_to_bin

# to support webp
Copy-Item "$copy_from_bin\libsharpyuv.dll" -Destination $copy_to_bin
Copy-Item "$copy_from_bin\libwebp.dll" -Destination $copy_to_bin
Copy-Item "$copy_from_bin\libwebpdecoder.dll" -Destination $copy_to_bin
Copy-Item "$copy_from_bin\libwebpdemux.dll" -Destination $copy_to_bin
Copy-Item "$copy_from_bin\libwebpmux.dll" -Destination $copy_to_bin
