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

bool_t
str_has_prefix(const char *s, const char *prefix)
{
  for (const char *p = s, *q = prefix; *q != '\0'; p++, q++) {
    if (*p != *q) {
      return FALSE;
    }
  }
  return TRUE;
}

char *
str_trim_prefix(char *s, const char *prefix)
{
  int32_t i;
  for (i = 0; prefix[i] != '\0'; i++) {
    if (s[i] != prefix[i]) {
      return s;
    }
  }
  return s + i;
}

char *
str_to_upper(char *s)
{
  for (char *p = s; *p != '\0'; p++) {
    if ('a' <= *p && *p <= 'z') {
      *p -= 0x20;
    }
  }
  return s;
}

int32_t
str_index(const char *s, char c)
{
  int32_t i;
  for (i = 0; s[i]; i++) {
    if (s[i] == c) {
      return i;
    }
  }
  return -1;
}
