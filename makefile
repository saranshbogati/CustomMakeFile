CC = g++
CFLAGS = -Wall -std=c++20 -pedantic

# Target Rules
##This is also a comment 
gulegede: gula.o geda.o test.o
    ${CC} ${CFLAGS} $^ -o $@

gula.o: gula.cpp
    ${CC} ${CFLAGS} -c $< -o $@

geda.o: geda.cpp
    @echo "Mero cha"
    ${CC} ${CFLAGS} -c $< -o $@

test.o: test.cpp
    ${CC} ${CFLAGS} -c $< -o $@
clean:
    rm *.o gulegede main
# Inference Rule
%.o: %.cpp
    ${CC} ${CFLAGS} -c $< -o $@
