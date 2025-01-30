#ifndef _GAME_H
#define _GAME_H


typedef struct Game_t Game;

Game* game_create();

void game_run(Game* game);

void game_setup(Game* game);

void game_destroy(Game* game);

// As long as framebuffer size and window size are equal,
// this is the one that matters.
void game_frame_buffer_size_changed(Game* game, int width, int height);

// There are aparently cases (MacOSX retina displays?, Windows DPI settings?)
// where pixel size and window size are not the same, and in that case,
// this callback is relevant.
//
// Simply log window size changes for now
void game_window_size_changed(Game* game, int width, int height);

#endif // GAME_H
