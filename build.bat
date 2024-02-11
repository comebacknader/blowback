@echo off

cl /Zi win32_blowback.cpp /link user32.lib shell32.lib opengl32.lib gdi32.lib /SUBSYSTEM:WINDOWS