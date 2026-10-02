@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "PYEXE="
where py >nul 2>&1 && set "PYEXE=py -3"
if not defined PYEXE (
  where python >nul 2>&1 && set "PYEXE=python"
)
if not defined PYEXE (
  echo Nao encontrei Python. Instale Python 3.13 e marque "Add python.exe to PATH" e "tcl/tk and IDLE".
  exit /b 1
)

echo Usando %PYEXE%
%PYEXE% -m pip install -r "%~dp0requirements.txt" pyinstaller
if errorlevel 1 exit /b 1

%PYEXE% -m PyInstaller --noconfirm --clean --onefile --windowed --name SimuladorCAN --paths "%~dp0" "%~dp0simulador_can.py"
if errorlevel 1 exit /b 1

echo.
echo Pronto: %~dp0dist\SimuladorCAN.exe
exit /b 0
