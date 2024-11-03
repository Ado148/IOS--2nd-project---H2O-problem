EXECUTABLE = proj2
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
default: $(EXECUTABLE) clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(EXECUTABLE).o
	$(CC) $(CFLAGS) -o $@ $^

proj2.o: $(EXECUTABLE).c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o