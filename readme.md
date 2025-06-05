# Terminal MP3 Player

A lightweight, cross-platform terminal-based MP3 player written in C. This player provides essential music playback functionality with a simple keyboard interface, perfect for command-line enthusiasts who want a no-frills music experience.

## Features

- **Playlist Management**: Automatically loads all MP3 files from a specified directory
- **Playback Controls**: Play, pause, stop, next, and previous track functionality
- **Real-time Display**: Shows current playlist with visual indicator for the playing song
- **Multi-threaded**: Separate audio thread ensures smooth playback without blocking user input
- **Cross-platform**: Works on Linux systems with PulseAudio
- **Memory Safe**: Proper cleanup and error handling throughout

## Dependencies

This player requires the following libraries:

- **libmpg123**: MP3 decoding library
- **libpulse**: PulseAudio library for audio output
- **pthread**: POSIX threads (usually included with GCC)

## Installation

### Ubuntu/Debian
```bash
make install-deps
make
```

### Fedora/RHEL/CentOS
```bash
make install-deps-fedora
make
```

### Arch Linux
```bash
make install-deps-arch
make
```

### Manual Installation
If you prefer to install dependencies manually:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libmpg123-dev libpulse-dev build-essential
```

**Fedora/RHEL:**
```bash
sudo dnf install mpg123-devel pulseaudio-libs-devel gcc
```

**Arch Linux:**
```bash
sudo pacman -S mpg123 libpulse gcc
```

Then compile:
```bash
gcc -Wall -Wextra -std=c99 -pthread -o mp3player mp3player.c -lmpg123 -lpulse-simple -lpulse -lpthread
```

## Usage

```bash
./mp3player /path/to/music/directory
```

**Example:**
```bash
./mp3player ~/Music
./mp3player /home/user/Downloads/music
```

The player will:
1. Scan the specified directory for MP3 files
2. Display the playlist
3. Automatically start playing the first song
4. Show available controls

## Controls

```
| Key     | Action                        |
|---------|-------------------------------|
| `SPACE` | Play/Pause current track      |
| `n`     | Next song                     |
| `p`     | Previous song                 |
| `l`     | List/refresh playlist display |
| `s`     | Stop playback                 |
| `q`     | Quit player                   |
```

## File Structure

```
mp3player/
├── mp3player.c        # Main source code
├── mp3player.h        # Header file with declarations
├── Makefile          # Build configuration
└── README.md         # This file
```

## Technical Details

### Architecture
- **Multi-threaded Design**: Separate threads for audio playback and user input handling
- **Thread Safety**: Mutex-protected shared state between threads
- **Non-blocking I/O**: Keyboard input doesn't interrupt audio playback
- **Memory Management**: Proper allocation and cleanup of resources

### Audio Pipeline
1. **Decoding**: libmpg123 handles MP3 decoding
2. **Output**: PulseAudio manages audio output
3. **Buffering**: 4KB buffer size for smooth playback
4. **Format**: Automatic format detection and conversion

### Supported Formats
- MP3 files (`.mp3`, `.MP3`)
- Various MP3 encoding formats supported by libmpg123

## Limitations

- **Audio System**: Requires PulseAudio (common on most Linux distributions)
- **File Format**: Currently supports MP3 files only
- **Directory Scanning**: Non-recursive (doesn't scan subdirectories)
- **Playlist Size**: Limited to 1000 songs maximum
- **Path Length**: Maximum 512 characters per file path

## Troubleshooting

### Common Issues

**"PulseAudio error" or no sound:**
- Ensure PulseAudio is running: `pulseaudio --check`
- Restart PulseAudio: `pulseaudio --kill && pulseaudio --start`
- Check audio output devices: `pactl list short sinks`

**"Error opening file":**
- Verify the MP3 file isn't corrupted
- Check file permissions
- Ensure the file path doesn't exceed 512 characters

**"Cannot open directory":**
- Verify the directory exists and is readable
- Check directory permissions
- Use absolute paths if relative paths cause issues

**Compilation errors:**
- Ensure all dependencies are installed
- Check that development headers are available (`-dev` packages)
- Verify GCC and make are installed

### Debug Mode
Uncomment the `printf("DEBUG: ...)` lines in the source code to enable debug output for troubleshooting playlist loading issues.

## Building from Source

```bash
# Clone or download the source
# Install dependencies (see Installation section)
make clean  # Remove any existing build files
make        # Build the player
```

---

**Note**: This player is designed for terminal/command-line use and requires a text-based interface. It will not work in GUI-only environments without terminal access.