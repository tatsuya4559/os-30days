#pragma once

#include "types.h"

#define NUM_DISK_SECTORS 2880

typedef struct {
  uint8_t name[8], ext[3], type;
  uint8_t reserve[10];
  uint16_t time, date, clustno;
  uint32_t size;
} FileInfo;

/**
 * Convert finfo to normalized filename.
 *
 * @param finfo file information
 * @param buf buffer to store the normalized filename.
 *        The buffer must be at least 13 bytes long.
 */
void file_normalized_name(const FileInfo *file, char *buf);
void file_readfat(uint8_t *fat, uint8_t *img);
void file_load(uint16_t clustno, uint32_t size, uint8_t *buf, uint8_t *fat, uint8_t *img);

typedef struct {
  FileInfo *files;
  int32_t index;
} FileInfoIterator;
FileInfo *next_file(FileInfoIterator *iter);
FileInfo *search_file(FileInfo *files, const char *filename);
