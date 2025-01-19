#pragma once

#include "types.h"

int32_t str_len(const char *s);
bool_t str_equal(const char *s1, const char *s2);
/**
 * Copy at most n characters from src to dest.
 * If src is shorter than n, dest equals to src.
 *
 * @param dest destination buffer
 * @param src source buffer
 * @param n maximum number of characters to copy
 * @return number of characters copied
 */
int32_t str_ncpy(char *dest, const char *src, int32_t n);
/**
 * Check if s has prefix.
 *
 * @param s string to check
 * @param prefix prefix to check
 * @return TRUE if s has prefix, FALSE otherwise
 */
bool_t str_has_prefix(const char *s, const char *prefix);
/**
 * Trim prefix from s.
 * If s does not have prefix, return s.
 *
 * @param s string to trim
 * @param prefix prefix to trim
 * @return pointer to the first character after prefix
 */
char *str_trim_prefix(char *s, const char *prefix);
/**
 * Capitalize s.
 * This function modifies s in place.
 *
 * @param s string to capitalize
 */
char *str_to_upper(char *s);
int32_t str_index(const char *s, char c);
