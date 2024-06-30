#include "GameWindow.h"

#include <stdio.h>

#define PLAYER_RECT_SIZE 50

static LRESULT CALLBACK game_window_process(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    // TODO: Make an input binding system.
  case WM_KEYDOWN:
    {
      GET_KEYSTATE_LPARAM(lParam);
      if ((lParam & KF_REPEAT) == 0)
      {
        // TODO: Register the action.
      }
      break;
    }
  case WM_KEYUP:
    {
      // TODO: Register the action.
      break;
    }
  case WM_LBUTTONDOWN:
    break;
  case WM_LBUTTONUP:
    break;
  case WM_RBUTTONDOWN:
    break;
  case WM_RBUTTONUP:
    break;
  case WM_MBUTTONDOWN:
    break;
  case WM_MBUTTONUP:
    break;
  case WM_XBUTTONDOWN:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
    {
    }
    else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
    {
    }
    break;
  case WM_XBUTTONUP:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
    {
    }
    else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
    {
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

