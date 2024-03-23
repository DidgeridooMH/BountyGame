#include "GameWindow.h"

#include "Client.h"
#include "Input/Input.h"
#include "Otter/GameState/Player/Player.h"

#define PLAYER_RECT_SIZE 50

static LRESULT CALLBACK game_window_process(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_KEYDOWN:
    switch (wParam)
    {
    case 'A':
      g_input.left = true;
      break;
    case 'D':
      g_input.right = true;
      break;
    case 'W':
      g_input.up = true;
      break;
    case 'S':
      g_input.down = true;
      break;
    }
    break;
  case WM_KEYUP:
    switch (wParam)
    {
    case 'A':
      g_input.left = false;
      break;
    case 'D':
      g_input.right = false;
      break;
    case 'W':
      g_input.up = false;
      break;
    case 'S':
      g_input.down = false;
      break;
    }
    break;
  case WM_CLOSE:
    DestroyWindow(window);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(window, message, wParam, lParam);
  }

  return 0;
}

HWND game_window_create(int width, int height, enum WindowMode windowMode)
{
  static const wchar_t CLASS_NAME[] = L"SampleWindowClass";

  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  HINSTANCE instance = GetModuleHandle(NULL);

  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = &game_window_process;
  wc.hInstance     = instance;
  wc.lpszClassName = CLASS_NAME;
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&wc);

  int windowFlags     = WS_OVERLAPPED;
  int windowPositionX = 0;
  int windowPositionY = 0;
  switch (windowMode)
  {
  case WM_WINDOWED:
    windowFlags |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    windowPositionX = CW_USEDEFAULT;
    windowPositionY = CW_USEDEFAULT;
    break;
  case WM_BORDERLESS_WINDOWED:
    windowFlags |= WS_POPUP;
    break;
  }

  HWND window = CreateWindowEx(0, CLASS_NAME, L"Otter Engine", windowFlags,
      windowPositionX, windowPositionY, width, height, NULL, NULL, instance,
      NULL);
  if (window == NULL)
  {
    return 0;
  }

  ShowWindow(window, SW_SHOWDEFAULT);
  UpdateWindow(window);

  return window;
}

void game_window_destroy(HWND window)
{
  DestroyWindow(window);
}

bool game_window_process_message()
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.message == WM_QUIT;
}
