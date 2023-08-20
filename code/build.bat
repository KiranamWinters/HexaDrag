@echo off

mkdir ..\build
pushd ..\build

cl -FC -Zi ..\code\win32_game.cpp user32.lib Gdi32.lib onecore.lib Kernel32.lib

call run.bat

popd






