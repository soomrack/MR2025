@echo off
cd /d C:\Users\Aleksandr\Documents\PolytechProg\OOP\p2p_messenger

echo Building...
g++ messenger.cpp -o messenger.exe -lws2_32

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build OK, launching...
start wt -w 0 nt --title "P2P Server" cmd /k "cd /d C:\Users\Aleksandr\Documents\PolytechProg\OOP\p2p_messenger && messenger.exe --role server"
start wt -w 0 nt --title "P2P Client" cmd /k "cd /d C:\Users\Aleksandr\Documents\PolytechProg\OOP\p2p_messenger && messenger.exe --role client"