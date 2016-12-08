# Compiler
CC = gcc

# Compiler flags
CFLAGS = -lrt -pthread

# Targets
TARGETS = printServer hackerNode node

all: $(TARGETS)
	$(RM) .nodes
	./reset.sh

.o:
	$(CC) -o $@ $(CFLAGS)

clean:
	$(RM) $(TARGETS)
	$(RM) .nodes
	./reset.sh
