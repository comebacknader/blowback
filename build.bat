@echo off

set common_compiler_flags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4244 -wd4201 -wd4100 -wd4189 -wd4505 -DBLOWBACK_INTERNAL=1 
set common_linker_flags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib /SUBSYSTEM:WINDOWS

pushd "build"
cl %common_compiler_flags% "..\win32_blowback.cpp" /link %common_linker_flags%
popd "build"