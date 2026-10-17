@echo off
rem Double-click or run: scripts\build-ps5.cmd            (build the hub app)
rem                      scripts\build-ps5.cmd deploy IP  (build and send by FTP)
rem The console test app: set APP=app, then run this script.
setlocal
if "%APP%"=="" set APP=hub
cd /d "%~dp0.."
wsl -d Ubuntu -- env APP=%APP% bash scripts/build-ps5.sh %*
