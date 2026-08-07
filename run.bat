@echo off
REM --- 1. ディレクトリの移動 ---
REM %USERPROFILE%はC:\Users\<koyama>を指します
set PROJECT_DIR=%USERPROFILE%\my-projects\air_guitar
cd /d "%PROJECT_DIR%"

REM --- 2. 仮想環境の有効化 ---
REM WindowsではScripts\activate.bat を実行する必要があります
call %PROJECT_DIR%\air_guitar001\Scripts\activate.bat

REM --- 3. 2つのPythonプロセスを同時に実行 ---
REM start "" は新しいウィンドウを開いてコマンドを実行し、同時に複数のプロセスを実行します
start "" python pose.py
start "" python guitar_dash.py

echo.
echo 2つのプログラムを別ウィンドウで起動しました。
echo これらのウィンドウを閉じるか、Ctrl+C (Batchファイル側) で終了してください。
pause