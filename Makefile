CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
LIBS = -lmpg123 -lpulse-simple -lpulse -lpthread

TARGET = mp3player
SOURCE = mp3player.c

# Default target
all: $(TARGET)

# Build the MP3 player
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

# Clean build files
clean:
	rm -f $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install libmpg123-dev libpulse-dev build-essential

# Install dependencies (Fedora/RHEL)
install-deps-fedora:
	sudo dnf install mpg123-devel pulseaudio-libs-devel gcc

# Install dependencies (Arch Linux)
install-deps-arch:
	sudo pacman -S mpg123 libpulse gcc

.PHONY: all clean install-deps install-deps-fedora install-deps-arch