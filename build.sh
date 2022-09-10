#!/bin/bash
# file: build.sh
echo -e "\e[93mC-Wordle Game - Coded By v3l0r3k\e[0m"
echo
echo [+] Attempting to compile game...
echo gcc cw.c -Wall -Wextra -o cw
gcc cw.c -Wall -Wextra -o cw
echo
echo -e "Run as \e[93m./cw\e[0m" 
echo Enjoy it!

