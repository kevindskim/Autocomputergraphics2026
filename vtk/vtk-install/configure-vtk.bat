@echo off
setlocal

cmake -S "D:\dev\VTK\vtk-src" ^
  -B "D:\dev\VTK\vtk-build" ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_INSTALL_PREFIX:PATH="D:\dev\VTK\vtk-install" ^
  -DCMAKE_PREFIX_PATH:PATH="D:\dev\Qt\6.10.2\msvc2022_64" ^
  -DVTK_GROUP_ENABLE_Qt=YES ^
  -DVTK_QT_VERSION=6 ^
  -DVTK_BUILD_TESTING=OFF ^
  -DBUILD_SHARED_LIBS=ON

if errorlevel 1 (
    echo VTK configuration failed.
    exit /b 1
)

echo VTK configuration completed.
endlocal