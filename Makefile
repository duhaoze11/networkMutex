# Compiler
CC = gcc

# Compiler flags
CFLAGS = -lrt -pthread

# Targets
TARGETS = printServer hackerNode node

all: $(TARGETS)

.o:
	$(CC) -o $@ $(CFLAGS)

clean:
	$(RM) $(TARGETS)
