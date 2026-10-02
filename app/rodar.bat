@echo off
setlocal EnableExtensions
cd /d "%~dp0"

where pythonw >nul 2>&1
if %errorlevel%==0 (
  start "" pythonw "%~dp0simulador_can.py"
  exit /b 0
)

where pyw >nul 2>&1
if %errorlevel%==0 (
  start "" pyw -3 "%~dp0simulador_can.py"
  exit /b 0
)

where python >nul 2>&1
if %errorlevel%==0 (
  python "%~dp0simulador_can.py"
  exit /b 0
)

echo Nao encontrei Python. Instale Python 3.13 com tcl/tk.
exit /b 1
