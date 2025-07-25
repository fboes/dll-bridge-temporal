@echo off
echo ========================================
echo  AEROFLY BRIDGE DLL - COMPILADOR
echo ========================================

REM Buscar Visual Studio 2022
set VS_PATH=""
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo  Visual Studio 2022 Community encontrado
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo  Visual Studio 2019 Community encontrado
) else (
    echo  Visual Studio no encontrado
    echo Instala Visual Studio Community con herramientas C++
    pause
    exit /b 1
)

REM Verificar archivos fuente
if not exist "aerofly_bridge_dll_complete_estable.cpp" (
    echo  Archivo aerofly_bridge_dll_complete_estable.cpp no encontrado
    pause
    exit /b 1
)

if not exist "tm_external_message.h" (
    echo  Archivo tm_external_message.h no encontrado
    pause
    exit /b 1
)

echo  Archivos fuente encontrados

REM Configurar entorno de compilación
echo  Configurando entorno de compilación...
call %VS_PATH%

REM Compilar la DLL
echo  Compilando AeroflyBridge.dll...
cl /LD /EHsc /O2 /std:c++17 /DWIN32 /D_WINDOWS /D_USRDLL ^
   aerofly_bridge_dll_complete_estable.cpp ^
   /Fe:AeroflyBridge.dll ^
   /link ws2_32.lib advapi32.lib

REM Verificar resultado
if exist "AeroflyBridge.dll" (
    echo  ¡Compilación exitosa!
    echo  DLL creada: AeroflyBridge.dll
    
    REM Limpiar archivos temporales
    del *.obj *.exp *.lib 2>nul
    
    echo.
    echo  Próximos pasos:
    echo 1. Copia AeroflyBridge.dll a: %%USERPROFILE%%\Documents\Aerofly FS 4\external_dll\
    echo 2. Ejecuta Aerofly FS 4
    echo 3. Inicia un vuelo
    echo 4. Prueba la conexión con Python
    echo.
) else (
    echo  Error en la compilación
    echo Revisa los mensajes de error arriba
)

pause