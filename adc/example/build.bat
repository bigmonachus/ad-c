@echo off

:: Compile metaprogram
cl metaprogram.c /Zi -I ..\

:: Generate code
metaprogram.exe

:: Compile actual program
cl program.c /Zi
