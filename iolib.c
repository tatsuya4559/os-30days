#include "types.h"
#include "strutil.h"
#include "iolib.h"

static
int32_t
dec2asc(char *str, int32_t dec)
{
  int32_t len = 0, len_buf; //桁数
  int32_t buf[10];
  while (1) { //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
    buf[len++] = dec % 10;
    if (dec < 10) break;
    dec /= 10;
  }
  len_buf = len;
  while (len) {
    *(str++) = buf[--len] + 0x30;
  }
  return len_buf;
}

//16進数からASCIIコードに変換
static
int32_t
hex2asc(char *str, int32_t dec)
{ //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
  int32_t len = 0, len_buf; //桁数
  int32_t buf[10];
  while (1) {
    buf[len++] = dec % 16;
    if (dec < 16) break;
    dec /= 16;
  }
  len_buf = len;
  while (len) {
    len --;
    *(str++) = (buf[len]<10)?(buf[len] + 0x30):(buf[len] - 9 + 0x60);
  }
  return len_buf;
}

static
int32_t
str_append(char *dest, const char *src)
{
  int32_t len = 0;
  for (const char *p = src; *p; p++) {
    *(dest++) = *p;
    len++;
  }
  return len;
}

void
sprintf(char *str, char *fmt, ...)
{
  va_list list;
  int32_t i, len;
  va_start(list, 2);

  while (*fmt) {
    if(*fmt=='%') {
      fmt++;
      switch(*fmt){
        case 'd':
          len = dec2asc(str, va_arg(list, int));
          break;
        case 'x':
          len = hex2asc(str, va_arg(list, int));
          break;
        case 's':
          len = str_append(str, va_arg(list, char *));
          break;
      }
      str += len; fmt++;
    } else {
      *(str++) = *(fmt++);
    }
  }
  *str = 0x00; //最後にNULLを追加
  va_end(list);
}
