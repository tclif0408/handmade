@echo off

mkdir ..\..\build
pushd ..\..\build

cl -Zi ..\source\win32_handmade.cpp user32.lib gdi32.lib

popd
