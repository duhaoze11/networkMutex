# Compiler
CC = gcc

# Compiler flags
CFLAGS = -lrt -pthread

# Targets
TARGETS = printServer hackerNode

all: $(TARGETS)

.o:
	$(CC) -o $@ $(CFLAGS)

clean:
	$(RM) $(TARGETS)
