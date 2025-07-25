@echo off
echo ========================================
echo  AEROFLY BRIDGE DLL - COMPILER
echo ========================================

REM Search for Visual Studio 2022
set VS_PATH=""
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo  Visual Studio 2022 Community found
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo  Visual Studio 2019 Community found
) else (
    echo  Visual Studio not found
    echo Please install Visual Studio Community with C++ tools
    pause
    exit /b 1
)

REM Verify source files
if not exist "aerofly_bridge_dll_complete_estable.cpp" (
    echo  File aerofly_bridge_dll_complete_estable.cpp not found
    pause
    exit /b 1
)

if not exist "tm_external_message.h" (
    echo  File tm_external_message.h not found
    pause
    exit /b 1
)

echo  Source files found

REM Setup compilation environment
echo  Setting up compilation environment...
call %VS_PATH%

REM Compile the DLL
echo  Compiling AeroflyBridge.dll...
cl /LD /EHsc /O2 /std:c++17 /DWIN32 /D_WINDOWS /D_USRDLL ^
   aerofly_bridge_dll_complete_estable.cpp ^
   /Fe:AeroflyBridge.dll ^
   /link ws2_32.lib advapi32.lib shell32.lib

REM Verify result
if exist "AeroflyBridge.dll" (
    echo  Compilation successful!
    echo  DLL created: AeroflyBridge.dll
    
    REM Clean temporary files
    del *.obj *.exp *.lib 2>nul
    
    echo.
    echo  Next steps:
    echo 1. Copy AeroflyBridge.dll to: %%USERPROFILE%%\Documents\Aerofly FS 4\external_dll\
    echo 2. Start Aerofly FS 4
    echo 3. Begin a flight
    echo 4. Test connection with Python
    echo.
) else (
    echo  Compilation error
    echo Check error messages above
)

pause