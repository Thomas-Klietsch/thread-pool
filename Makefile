# Simple makefile

CC := g++ -std=c++17 -O3

CCW := -Wall -Werror -Wextra

.DEFAULT_GOAL := main.cpp

main.cpp:
	mkdir -p bin
	$(CC) $(CCW) -o ./bin/main ./src/main.cpp
