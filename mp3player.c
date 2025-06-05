#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include "mp3player.h"

Player player = {0};
Playlist playlist = {0};
struct termios old_termios;

void cleanup()
{
    pthread_mutex_lock(&player.control_mutex);
    player.should_stop = 1;
    pthread_mutex_unlock(&player.control_mutex);

    if (player.audio_thread)
    {
        pthread_join(player.audio_thread, NULL);
    }

    if (player.mh)
    {
        mpg123_close(player.mh);
        mpg123_delete(player.mh);
    }

    if (player.pa)
    {
        pa_simple_free(player.pa);
    }

    mpg123_exit();

    if (playlist.songs)
    {
        for (int i = 0; i < playlist.count; i++)
        {
            if (playlist.songs[i])
            {
                free(playlist.songs[i]);
            }
        }
        free(playlist.songs);
    }

    pthread_mutex_destroy(&player.control_mutex);
    restore_terminal();
}

void signal_handler(int sig __attribute__((unused)))
{
    cleanup();
    exit(0);
}

int setup_terminal()
{
    struct termios new_termios;

    if (tcgetattr(STDIN_FILENO, &old_termios) != 0)
    {
        return -1;
    }

    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0)
    {
        return -1;
    }

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    return 0;
}

void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

int kbhit()
{
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0;
}

int getch()
{
    return getchar();
}

void load_playlist(const char *directory)
{
    DIR *dir;
    struct dirent *entry;
    char filepath[MAX_PATH];

    dir = opendir(directory);
    if (!dir)
    {
        printf("Error: Cannot open directory %s\n", directory);
        return;
    }

    playlist.songs = calloc(MAX_SONGS, sizeof(char *));
    if (!playlist.songs)
    {
        printf("Error: Memory allocation failed\n");
        closedir(dir);
        return;
    }

    playlist.count = 0;

    while ((entry = readdir(dir)) != NULL && playlist.count < MAX_SONGS)
    {
        if (strstr(entry->d_name, ".mp3") || strstr(entry->d_name, ".MP3"))
        {
            int len = snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
            if (len >= (int)sizeof(filepath))
            {
                printf("Warning: Path too long, skipping: %s/%s\n", directory, entry->d_name);
                continue;
            }

            size_t path_len = strlen(filepath) + 1;
            playlist.songs[playlist.count] = malloc(path_len);
            if (playlist.songs[playlist.count] == NULL)
            {
                printf("Error: Memory allocation failed for song %d\n", playlist.count);
                break;
            }
            strcpy(playlist.songs[playlist.count], filepath);
            playlist.count++;
        }
    }

    closedir(dir);
    playlist.current = 0;
    printf("Loaded %d MP3 files from %s\n\n\n", playlist.count, directory);
}

void print_playlist()
{
    printf("\n==================== PLAYLIST ====================\n");

    if (playlist.count == 0 || playlist.songs == NULL)
    {
        printf("No songs in playlist\n");
    }
    else
    {
        for (int i = 0; i < playlist.count; i++)
        {
            if (playlist.songs[i] == NULL)
            {
                printf("   %d. [NULL]\n", i + 1);
                continue;
            }

            char *filename = strrchr(playlist.songs[i], '/');
            filename = filename ? filename + 1 : playlist.songs[i];

            if (i == playlist.current)
            {
                printf("-> %d. %s\n", i + 1, filename);
            }
            else
            {
                printf("   %d. %s\n", i + 1, filename);
            }
        }
    }
    printf("===================================================\n\n");
}

void print_controls()
{
    printf("\n======== CONTROLS ========\n");
    printf("SPACE - Play/Pause\n");
    printf("n     - Next song\n");
    printf("p     - Previous song\n");
    printf("l     - List playlist\n");
    printf("s     - Stop\n");
    printf("q     - Quit\n");
    printf("==========================\n\n");
}

void *audio_thread_func(void *arg __attribute__((unused)))
{
    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    int error;

    while (1)
    {
        pthread_mutex_lock(&player.control_mutex);
        if (player.should_stop)
        {
            pthread_mutex_unlock(&player.control_mutex);
            break;
        }

        if (!player.is_playing || player.paused)
        {
            pthread_mutex_unlock(&player.control_mutex);
            usleep(10000); // 10ms
            continue;
        }
        pthread_mutex_unlock(&player.control_mutex);

        if (mpg123_read(player.mh, buffer, BUFFER_SIZE, &bytes_read) == MPG123_OK)
        {
            if (bytes_read > 0)
            {
                if (pa_simple_write(player.pa, buffer, bytes_read, &error) < 0)
                {
                    printf("PulseAudio write error: %s\n", pa_strerror(error));
                    break;
                }
            }
        }
        else
        {
            // End of file, move to next song
            next_song();
        }
    }
    return NULL;
}

int play_song(const char *filename)
{
    long rate;
    int channels, encoding;
    pa_sample_spec ss;
    int error;

    // Stop current playback
    pthread_mutex_lock(&player.control_mutex);
    player.should_stop = 1;
    pthread_mutex_unlock(&player.control_mutex);

    if (player.audio_thread)
    {
        pthread_join(player.audio_thread, NULL);
        player.audio_thread = 0;
    }

    if (player.pa)
    {
        pa_simple_free(player.pa);
        player.pa = NULL;
    }

    if (player.mh)
    {
        mpg123_close(player.mh);
        mpg123_delete(player.mh);
    }

    // Initialize new decoder
    player.mh = mpg123_new(NULL, &error);
    if (!player.mh)
    {
        printf("Error creating mpg123 handle: %s\n", mpg123_plain_strerror(error));
        return -1;
    }

    mpg123_param(player.mh, MPG123_ADD_FLAGS, MPG123_QUIET, 0);

    if (mpg123_open(player.mh, filename) != MPG123_OK)
    {
        printf("Error opening file: %s\n", filename);
        return -1;
    }

    if (mpg123_getformat(player.mh, &rate, &channels, &encoding) != MPG123_OK)
    {
        printf("Error getting format\n");
        return -1;
    }

    // Setup PulseAudio
    ss.format = PA_SAMPLE_S16LE;
    ss.rate = rate;
    ss.channels = channels;

    player.pa = pa_simple_new(NULL, "MP3 Player", PA_STREAM_PLAYBACK, NULL,
                              "playback", &ss, NULL, NULL, &error);
    if (!player.pa)
    {
        printf("PulseAudio error: %s\n", pa_strerror(error));
        return -1;
    }

    // Start playback
    pthread_mutex_lock(&player.control_mutex);
    player.is_playing = 1;
    player.paused = 0;
    player.should_stop = 0;
    pthread_mutex_unlock(&player.control_mutex);

    if (pthread_create(&player.audio_thread, NULL, audio_thread_func, NULL) != 0)
    {
        printf("Error creating audio thread\n");
        return -1;
    }

    char *basename = strrchr(filename, '/');
    basename = basename ? basename + 1 : (char *)filename;
    printf("Now playing: %s\n", basename);

    return 0;
}

void pause_toggle()
{
    pthread_mutex_lock(&player.control_mutex);
    if (player.is_playing)
    {
        player.paused = !player.paused;
        printf("%s\n", player.paused ? "Paused" : "Resumed");
    }
    pthread_mutex_unlock(&player.control_mutex);
}

void next_song()
{
    if (playlist.count > 0)
    {
        playlist.current = (playlist.current + 1) % playlist.count;
        play_song(playlist.songs[playlist.current]);
    }
}

void previous_song()
{
    if (playlist.count > 0)
    {
        playlist.current = (playlist.current - 1 + playlist.count) % playlist.count;
        play_song(playlist.songs[playlist.current]);
    }
}

void stop_playback()
{
    pthread_mutex_lock(&player.control_mutex);
    player.is_playing = 0;
    player.paused = 0;
    pthread_mutex_unlock(&player.control_mutex);
    printf("Stopped\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <music_directory>\n", argv[0]);
        printf("Example: %s ~/Music\n", argv[0]);
        return 1;
    }

    // Initialize
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (mpg123_init() != MPG123_OK)
    {
        printf("Error initializing mpg123\n");
        return 1;
    }

    if (pthread_mutex_init(&player.control_mutex, NULL) != 0)
    {
        printf("Error initializing mutex\n");
        return 1;
    }

    if (setup_terminal() != 0)
    {
        printf("Error setting up terminal\n");
        return 1;
    }

    // Load playlist
    load_playlist(argv[1]);
    if (playlist.count == 0)
    {
        printf("No MP3 files found in directory: %s\n", argv[1]);
        cleanup();
        return 1;
    }

    printf("==================== Terminal MP3 Player ====================\n");
    print_controls();

    // Only print playlist and start playing if we have songs
    if (playlist.count > 0 && playlist.songs[0] != NULL)
    {
        print_playlist();
        // Start playing first song
        play_song(playlist.songs[playlist.current]);
    }
    else
    {
        printf("Error: No valid songs loaded\n");
        cleanup();
        return 1;
    }

    // Main control loop
    int ch;
    while (1)
    {
        if (kbhit())
        {
            ch = getch();
            switch (ch)
            {
            case ' ': // Space - Play/Pause
                pause_toggle();
                break;
            case 'n': // Next song
                next_song();
                break;
            case 'p': // Previous song
                previous_song();
                break;
            case 'l': // List playlist
                print_playlist();
                break;
            case 's': // Stop
                stop_playback();
                break;
            case 'q': // Quit
                cleanup();
                return 0;
            default:
                break;
            }
        }
        usleep(50000); // 50ms delay
    }

    cleanup();
    return 0;
}