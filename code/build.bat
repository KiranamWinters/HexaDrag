@echo off

mkdir ..\build
pushd ..\build

cl -Zi ..\code\win32_game.cpp user32.lib

popd






