#ifndef MP3PLAYER_H
#define MP3PLAYER_H

#include <mpg123.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pthread.h>

#define MAX_SONGS 1000
#define MAX_PATH 512
#define BUFFER_SIZE 4096

typedef struct
{
    char **songs;
    int count;
    int current;
} Playlist;

typedef struct
{
    mpg123_handle *mh;
    pa_simple *pa;
    int is_playing;
    int should_stop;
    int paused;
    pthread_t audio_thread;
    pthread_mutex_t control_mutex;
} Player;

// Function prototypes
void cleanup();
void signal_handler(int sig);
int setup_terminal();
void restore_terminal();
int kbhit();
int getch();
void load_playlist(const char *directory);
void print_playlist();
void print_controls();
void *audio_thread_func(void *arg);
int play_song(const char *filename);
void pause_toggle();
void next_song();
void previous_song();
void stop_playback();

#endif // MP3PLAYER_H