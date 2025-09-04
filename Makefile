
CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -I./src

# Create needed directories
$(shell mkdir -p build bin)

# Source files
SRCS = src/main.c src/runbox.c src/namespaces.c
OBJS = $(patsubst src/%.c,bin/%.o,$(SRCS))

# Build the executable
build/runbox: $(OBJS)
	$(CC) $(OBJS) -o build/runbox

# Compile source files to object files
bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin build
