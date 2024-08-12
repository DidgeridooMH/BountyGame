#include "Otter/Util/File.h"

#include "Otter/Util/Log.h"

char* file_load(const char* path, uint64_t* fileLength)
{
  FILE* file;
  if (fopen_s(&file, path, "rb") != 0)
  {
    LOG_ERROR("Failed to open file for reading: %s", path);
    return NULL;
  }

  _fseeki64(file, 0, SEEK_END);
  int64_t length = _ftelli64(file);
  rewind(file);

  if (length < 0)
  {
    LOG_ERROR("Failed to get file length: %s", path);
    fclose(file);
    return NULL;
  }

  char* text = malloc(length + 1);
  if (text == NULL)
  {
    LOG_ERROR("Failed to allocate memory for file: %s", path);
    fclose(file);
    return NULL;
  }

  text[length] = '\0';

  fread(text, 1, length, file);
  fclose(file);

  if (fileLength != NULL)
  {
    *fileLength = length;
  }

  LOG_DEBUG("Loaded file: %s (%llu)", path, length);

  return text;
}

void file_write(const char* path, const char* data, uint64_t length)
{
  FILE* file;
  if (fopen_s(&file, path, "w") != 0)
  {
    LOG_ERROR("Failed to open file for writing: %s", path);
    return;
  }

  fwrite(data, 1, length, file);
  fclose(file);
}

void file_get_executable_path(char* buffer, uint64_t bufferSize)
{
  GetModuleFileNameA(NULL, buffer, bufferSize);
  char* lastSlash = strrchr(buffer, '\\');
  if (lastSlash != NULL)
  {
    *(lastSlash + 1) = '\0';
  }
}
