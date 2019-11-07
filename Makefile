CXXFLAGS=-g -std=c++17 -I glad/include -Wall -Wextra -Weffc++
LDFLAGS=-lGL -lX11 -lglfw -ldl -lpthread
CC=g++

LD=g++

all: main

texture.o: texture.h texture.cpp
main.o: main.cpp texture.h

main: main.o texture.o glad/src/glad.c
