@echo off

cls

IF NOT EXIST build\ mkdir build\
pushd build\

cl -DHAMDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -Zi ..\source\win32_handmade.cpp user32.lib gdi32.lib

popd
