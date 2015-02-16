@echo off

:: Compile metaprogram
cl metaprogram.c /D_CRT_SECURE_NO_WARNINGS /W4 /Zi -I ..\

:: Generate code
metaprogram.exe

:: Compile actual program
cl program.c /W4 /Zi
