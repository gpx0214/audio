CD /d "%~dp0"

g++ "wav2mp3.cpp" -Wall -L . -lmp3lame -o "wav2mp3.exe"
pause
"wav2mp3.exe"
pause
