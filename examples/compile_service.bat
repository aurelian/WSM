@ECHO OFF
REM $Id: compile_service.bat,v 1.2 2005/08/25 09:57:45 aurelian Exp $
ECHO It compiles wsm sample service using the gcc compiler for Windows.
ECHO g++ binary must be placed on your PATH environment variable!
g++ -o wsm.exe service/service.cpp
ECHO Done!
