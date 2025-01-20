#include "strutil.h"
#include "file.h"

#define MAX_FILES 224

void
file_normalized_name(const FileInfo *file, char *buf)
{
  int32_t idx = 0;
  for (int32_t i = 0; i < 8; i++) {
    if (file->name[i] == ' ') {
      break;
    }
    buf[idx++] = file->name[i];
  }
  buf[idx++] = '.';
  for (int32_t i = 0; i < 3; i++) {
    if (file->ext[i] == ' ') {
      break;
    }
    buf[idx++] = file->ext[i];
  }
  buf[idx] = '\0';
  if (str_equal(buf, ".")) {
    buf[0] = '\0';
  }
}

void
file_readfat(uint8_t *fat, uint8_t *img)
{
  // Read 3 bytes and decompress to 2 bytes like below:
  // 1A 2B 3C -> B1A 3C2
  for (int32_t i = 0, j = 0; i < NUM_DISK_SECTORS; i += 2) {
    fat[i + 0] = (img[j + 0]) | (img[j + 1] << 8) & 0xfff;
    fat[i + 1] = (img[j + 1] >> 4) | (img[j + 2] << 4) & 0xfff;
    j += 3;
  }
}

void
file_load(uint16_t clustno, uint32_t size, uint8_t *buf, uint8_t *fat, uint8_t *img)
{
  while (size > 0) {
    uint32_t batch_size = size > 512 ? 512 : size;
    for (uint32_t i = 0; i < batch_size; i++) {
      buf[i] = img[clustno * 512 + i];
    }
    size -= batch_size;
    buf += batch_size;
    clustno = fat[clustno];
  }
}

FileInfo *
next_file(FileInfoIterator *iter)
{
  if (iter->index >= MAX_FILES) {
    return NULL;
  }
  for (;;) {
    FileInfo *next = &iter->files[iter->index++];
    if (next->name[0] == 0x00) {
      // If the first byte of the name is 0x00, there are no more files.
      return NULL;
    }
    if (next->name[0] == 0xe5) {
      // If the first byte of the name is 0xe5, the file is deleted.
      continue;
    }
    return next;
  }
  return NULL;
}

FileInfo *
search_file(FileInfo *files, const char *filename)
{
  char name[13];
  FileInfoIterator iter = {
    .files = files,
    .index = 0,
  };
  FileInfo *file;
  while ((file = next_file(&iter)) != NULL) {
    file_normalized_name(file, name);
    if (str_equal(name, filename)) {
      return file;
    }
  }
  return NULL;
}
