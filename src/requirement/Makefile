CC = gcc
CFLAGS = -Wall -g

# Object files
OBJ = cmpsh.o

# Executable name
EXEC = cmpsh

# Build the shell
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC)

# Compile cmpsh.c into object file
cmpsh.o: cmpsh.c
	$(CC) $(CFLAGS) -c cmpsh.c

# Clean up
clean:
	rm -f $(OBJ) $(EXEC)
