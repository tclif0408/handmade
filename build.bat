@echo off

cls

set CommonCompilerFlags=-MT -Gm- -nologo -GR- -EHa- -Oi -WX -W4 -wd4100 -wd4189 -wd4127 -wd4505 -wd4701 -wd4201 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -Z7 -Fmwin32_handmade.map
set CommonLinkerFlags=-opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST build\ mkdir build\
pushd build\

rem -WX --> treat warnings as errors
rem -wd#### --> ignore warning ####

rem 32 BIT COMPILE
rem cl %CommonCompilerFlags ..\source\win32_handmade.cpp /link -subsystem:windows,"5.1" %CommonLinkerFlags

rem 64 BIT COMPILE
cl %CommonCompilerFlags% ..\source\win32_handmade.cpp /link %CommonLinkerFlags%

popd
