# Makefile Utility
This program behaves like what a `make` utility behaves.

## Compile
To compile the program do the following
```bash
g++ -Wall -std=c++20 -pedantic main.cpp -o main.o
```
This will compile the program with the output being `a.out`

## Run
To run the program, simply do the following 
```bash
./a.out #This runs the program like calling make. This will build all the targets

./a.out <custom_target> <options> #This only builds the custom target provided in the command
```


## Options
These are the options
1. `-f <file>`: Custom makefile
2. `-t N`: Timeout
3. `-d`: Debug flag
4: `-k`: Continue execution on exception
5. `-p`: Print the parsed makefile
6. `-d`: Print debug statements
