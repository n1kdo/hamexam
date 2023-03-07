@echo off
md stage
copy *.    stage > NUL
copy *.bat stage > NUL
copy *.bmp stage > NUL
copy *.c   stage > NUL
copt *.dat stage > NUL
copy *.def stage > NUL
copy *.dll stage > NUL
copy *.doc stage > NUL
copy *.dsw stage > NUL
copy *.exe stage > NUL
copy *.h   stage > NUL
copy *.hlp stage > NUL
copy *.ico stage > NUL
copy *.ide stage > NUL
copy *.mak stage > NUL
copy *.rc  stage > NUL
c:\bin\pkzip -a old\%1 stage\*.*
c:\dos\deltree /Y stage > nul

