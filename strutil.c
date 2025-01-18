#include "strutil.h"

int32_t
str_len(const char *s)
{
  int32_t len = 0;
  for (; s[len] != '\0'; len++) {}
  return len;
}

bool_t
str_equal(const char *s1, const char *s2)
{
  if (str_len(s1) != str_len(s2)) {
    return FALSE;
  }
  for (const char *p = s1, *q = s2; *p != '\0' && *q != '\0'; p++, q++) {
    if (*p != *q) {
      return FALSE;
    }
  }
  return TRUE;
}

int32_t
str_ncpy(char *dest, const char *src, int32_t n)
{
  int32_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  dest[i] = '\0';
  return i;
}
