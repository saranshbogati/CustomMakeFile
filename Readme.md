# Makefile Utility
This program behaves like what a `make` utility behaves.

## Compile
To compile the program do the following
```bash
g++ -Wall -std=c++20 -pedantic main.cpp
```
This will compile the program with the output being `a.out`

## Run
To run the program, simply do the following 
```bash
./a.out #This runs the program like calling make. This will build all the targets

./a.out <custom_target> #This only builds the custom target provided in the command
```

## Flags
To be done