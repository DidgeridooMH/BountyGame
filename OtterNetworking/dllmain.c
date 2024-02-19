#include "pch.h"

BOOL APIENTRY DllMain(
    HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      fprintf(stderr, "There was a problem starting the networking services.");
      return FALSE;
    }
    break;
  case DLL_PROCESS_DETACH:
    WSACleanup();
    break;
  default:
    break;
  }
  return TRUE;
}
