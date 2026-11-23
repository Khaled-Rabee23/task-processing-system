CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -pthread
LDFLAGS ?= -pthread

TARGET := build/task_processor
SOURCE := src/main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p build
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
