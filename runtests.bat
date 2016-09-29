@echo off
setlocal

call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat"

python scripts\tests.py %*
