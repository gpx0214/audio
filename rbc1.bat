CD /D "%~dp0"

:start
if %ERRORLEVEL%==1 goto end
ffmpeg -i http://bjcdnlive.rbc.cn/live/live1.m3u8 -vcodec copy -acodec copy "%DATE:~0,4%-%DATE:~5,2%-%DATE:~8,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.ts"
echo %ERRORLEVEL%
:pause
shift /1
goto start

:end
pause
