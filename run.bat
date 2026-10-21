@echo off
cd /d "%~dp0"
call "%~dp0air_guitar001\Scripts\activate.bat"
python "%~dp0supervise.py"
