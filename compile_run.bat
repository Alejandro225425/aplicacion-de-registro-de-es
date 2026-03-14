@echo off
chcp 65001 >nul
color 0B
echo.
echo  ===================================================================
echo   MONITOR DE CONTROLADORES E/S, DMA E INTERRUPCIONES DEL SO
echo   Compilador y Ejecutor - MinGW g++
echo  ===================================================================
echo.

REM Directorio del proyecto
set "PROYECTO=%~dp0"
cd /d "%PROYECTO%"

REM Verificar que g++ este disponible
where g++ >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] g++ no encontrado en el PATH del sistema.
    echo.
    echo  Asegurese de tener MinGW-w64 instalado y en el PATH.
    echo  Puede descargarlo en: https://www.mingw-w64.org/
    echo.
    pause
    exit /b 1
)

REM Mostrar version del compilador
echo  Compilador detectado:
g++ --version 2>&1 | findstr /i "g++"
echo.

REM Eliminar ejecutable anterior si existe
if exist "monitor_es.exe" del /f /q "monitor_es.exe"

REM Compilar todos los archivos .cpp
echo  Compilando archivos...
echo.

g++ -std=c++17 -O2 ^
    main.cpp ^
    device_monitor.cpp ^
    dma_monitor.cpp ^
    interrupt_monitor.cpp ^
    simulation_monitor.cpp ^
    -o monitor_es.exe ^
    -lsetupapi ^
    -lcfgmgr32 ^
    -lntdll ^
    2>&1

REM Verificar resultado
if errorlevel 1 (
    echo.
    echo  [ERROR] La compilacion fallo. Revise los mensajes de arriba.
    echo.
    pause
    exit /b 1
)

echo.
echo  [OK] Compilacion exitosa. Ejecutando monitor_es.exe...
echo.
timeout /t 1 /nobreak >nul

REM Ejecutar la aplicacion
monitor_es.exe

echo.
echo  Programa finalizado.
pause
