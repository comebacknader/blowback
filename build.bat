@echo off

cl /Zi main.cpp /I "G:\work\libs\SDL2\include" /link /LIBPATH:G:\work\libs\SDL2\lib\x64 user32.lib SDL2.lib SDL2main.lib shell32.lib opengl32.lib /SUBSYSTEM:CONSOLE