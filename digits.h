#ifndef DIGITS_H
#define DIGITS_H

#include <stdint.h>

// ============ NUMBER ARRAYS ============ //
extern const uint8_t font20x20[10][20][20];

// ============ DRAWING FUNCTIONS ============ //
void draw_pixel(int x, int y, int colour);
void draw_digit_20x20(int digit, int x, int y, int colour);
void draw_score_20x20(int score, int x, int y, int colour);
void draw_digit_60x60(int digit, int x, int y, int colour);
void draw_score_60x60(int score, int x, int y, int colour);
void restore_rect_from_background(int dst_x, int dst_y, int w, int h);
void draw_volume(int vol);
void draw_score(int score);
void clear_score(int score);
void clear_volume(int vol);
#endif // DIGITS_H
