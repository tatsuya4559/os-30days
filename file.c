#include "strutil.h"
#include "file.h"

void
file_normalized_name(const FileInfo *finfo, char *buf)
{
  int32_t idx = 0;
  for (int32_t i = 0; i < 8; i++) {
    if (finfo->name[i] == ' ') {
      break;
    }
    buf[idx++] = finfo->name[i];
  }
  buf[idx++] = '.';
  for (int32_t i = 0; i < 3; i++) {
    if (finfo->ext[i] == ' ') {
      break;
    }
    buf[idx++] = finfo->ext[i];
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
