#include "Otter/Util/File.h"

char* file_load(const char* path, uint64_t* fileLength)
{
  // TODO: Switch this to UNICODE standard.
  HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE)
  {
    return NULL;
  }

  LARGE_INTEGER fileSize = {0};
  fileSize.LowPart       = GetFileSize(file, (LPDWORD) &fileSize.HighPart);

  if (fileLength != NULL)
  {
    *fileLength = fileSize.QuadPart;
  }

  char* text = malloc(fileSize.QuadPart + 1);
  if (text == NULL)
  {
    CloseHandle(file);
    return NULL;
  }

  text[fileSize.QuadPart] = '\0';

  uint64_t totalRead = 0;
  while (fileSize.QuadPart > 0)
  {
    ULONG bytesRead = 0;
    if (!ReadFile(file, &text[totalRead], fileSize.LowPart, &bytesRead, NULL))
    {
      free(text);
      CloseHandle(file);
      return NULL;
    }
    fileSize.QuadPart -= bytesRead;
    totalRead += bytesRead;
  }

  return text;
}

