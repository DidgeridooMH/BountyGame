#pragma once

enum WindowMode
{
  WM_WINDOWED,
  WM_BORDERLESS_WINDOWED
};

HWND game_window_create(int width, int height, enum WindowMode windowMode);
void game_window_destroy(HWND window);
bool game_window_process_message(HWND window);
