#include <am.h>
#include <klib.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__ISA_NATIVE__)

extern Area heap;

typedef struct {
  size_t size;
} AllocHeader;

static uintptr_t heap_brk;
static unsigned long rand_seed = 1;
static int errno_slot;

FILE *stdout = (FILE *)(uintptr_t)1;
FILE *stderr = (FILE *)(uintptr_t)2;

static void *shim_alloc_raw(size_t size) {
  if (heap_brk == 0) {
    heap_brk = (uintptr_t)heap.start;
  }
  size = (size + 7) & ~(size_t)7;
  uintptr_t old = heap_brk;
  heap_brk += size;
  assert(heap_brk <= (uintptr_t)heap.end);
  return (void *)old;
}

static int ascii_tolower(int ch) {
  if (ch >= 'A' && ch <= 'Z') {
    return ch - 'A' + 'a';
  }
  return ch;
}

static int ascii_isspace(int ch) {
  switch (ch) {
    case ' ': case '\f': case '\n': case '\r': case '\t': case '\v':
      return 1;
    default:
      return 0;
  }
}

static int ascii_isdigit(int ch) {
  return ch >= '0' && ch <= '9';
}

static int ascii_isalpha(int ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static int ascii_isxdigit(int ch) {
  return ascii_isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static int ascii_xdigit(int ch) {
  if (ascii_isdigit(ch)) {
    return ch - '0';
  }
  ch = ascii_tolower(ch);
  return ch - 'a' + 10;
}

static void shim_write(const char *buf, int len) {
  int i;
  for (i = 0; i < len && buf[i]; ++i) {
    putch(buf[i]);
  }
}

static long shim_strto_ul(const char *nptr, char **endptr, int base, bool is_signed, bool *negative) {
  const char *p = nptr;
  unsigned long value = 0;
  bool neg = false;
  bool any = false;

  while (ascii_isspace(*p)) {
    ++p;
  }

  if (*p == '+' || *p == '-') {
    neg = (*p == '-');
    ++p;
  }

  if (base == 0) {
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
      base = 16;
      p += 2;
    } else if (p[0] == '0') {
      base = 8;
      ++p;
      any = true;
    } else {
      base = 10;
    }
  } else if (base == 16 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    p += 2;
  }

  while (*p) {
    int digit;
    if (!ascii_isxdigit(*p)) {
      break;
    }
    digit = ascii_xdigit(*p);
    if (digit >= base) {
      break;
    }
    value = value * (unsigned long)base + (unsigned long)digit;
    any = true;
    ++p;
  }

  if (!any) {
    p = nptr;
  }

  if (endptr) {
    *endptr = (char *)p;
  }
  if (negative) {
    *negative = neg && is_signed;
  }
  return (long)value;
}

static float shim_pow10(int exp) {
  float value = 1.0f;
  int i;
  if (exp >= 0) {
    for (i = 0; i < exp; ++i) {
      value *= 10.0f;
    }
  } else {
    for (i = 0; i < -exp; ++i) {
      value /= 10.0f;
    }
  }
  return value;
}

static int64_t days_from_civil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  int era = (year >= 0 ? year : year - 399) / 400;
  unsigned yoe = (unsigned)(year - era * 400);
  unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int64_t)doe - 719468;
}

static void civil_from_days(int64_t z, int *year, unsigned *month, unsigned *day) {
  z += 719468;
  int era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = (int)yoe + era * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;

  *day = doy - (153 * mp + 2) / 5 + 1;
  *month = mp + (mp < 10 ? 3 : -9);
  *year = y + (*month <= 2);
}

int rand(void) {
  rand_seed = rand_seed * 1103515245 + 12345;
  return (unsigned int)(rand_seed / 65536) % 32768;
}

void srand(unsigned int seed) {
  rand_seed = seed;
}

int abs(int x) {
  return x < 0 ? -x : x;
}

int atoi(const char *nptr) {
  return (int)strtol(nptr, NULL, 10);
}

void *malloc(size_t size) {
  AllocHeader *header = shim_alloc_raw(sizeof(*header) + size);
  header->size = size;
  return header + 1;
}

void free(void *ptr) {
  (void)ptr;
}

void *calloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  void *ptr = malloc(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

void *realloc(void *ptr, size_t size) {
  void *new_ptr;
  size_t copy_size;
  AllocHeader *header;

  if (!ptr) {
    return malloc(size);
  }
  if (size == 0) {
    return NULL;
  }

  header = ((AllocHeader *)ptr) - 1;
  new_ptr = malloc(size);
  if (!new_ptr) {
    return NULL;
  }
  copy_size = header->size < size ? header->size : size;
  memcpy(new_ptr, ptr, copy_size);
  return new_ptr;
}

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c) {
      return (char *)s;
    }
    ++s;
  }
  return c == 0 ? (char *)s : NULL;
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;
  do {
    if (*s == (char)c) {
      last = s;
    }
  } while (*s++);
  return (char *)last;
}

int strcasecmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    int c1 = ascii_tolower(*s1);
    int c2 = ascii_tolower(*s2);
    if (c1 != c2) {
      return c1 - c2;
    }
    ++s1;
    ++s2;
  }
  return ascii_tolower(*s1) - ascii_tolower(*s2);
}

long strtol(const char *nptr, char **endptr, int base) {
  bool negative = false;
  long value = shim_strto_ul(nptr, endptr, base, true, &negative);
  return negative ? -value : value;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
  bool negative = false;
  unsigned long value = (unsigned long)shim_strto_ul(nptr, endptr, base, false, &negative);
  return negative ? (unsigned long)(-(long)value) : value;
}

float strtof(const char *nptr, char **endptr) {
  const char *p = nptr;
  float value = 0.0f;
  float frac = 0.0f;
  float scale = 1.0f;
  int sign = 1;
  int exp_sign = 1;
  int exp = 0;
  bool any = false;

  while (ascii_isspace(*p)) {
    ++p;
  }
  if (*p == '+' || *p == '-') {
    sign = *p == '-' ? -1 : 1;
    ++p;
  }
  while (ascii_isdigit(*p)) {
    value = value * 10.0f + (float)(*p - '0');
    any = true;
    ++p;
  }
  if (*p == '.') {
    ++p;
    while (ascii_isdigit(*p)) {
      frac = frac * 10.0f + (float)(*p - '0');
      scale *= 10.0f;
      any = true;
      ++p;
    }
  }
  value += frac / scale;
  if ((*p == 'e' || *p == 'E') && any) {
    const char *exp_start = ++p;
    if (*p == '+' || *p == '-') {
      exp_sign = *p == '-' ? -1 : 1;
      ++p;
    }
    if (!ascii_isdigit(*p)) {
      p = exp_start - 1;
    } else {
      while (ascii_isdigit(*p)) {
        exp = exp * 10 + (*p - '0');
        ++p;
      }
      value *= shim_pow10(exp_sign * exp);
    }
  }
  if (endptr) {
    *endptr = (char *)(any ? p : nptr);
  }
  return sign * value;
}

locale_t newlocale(int category_mask, const char *locale, locale_t base) {
  (void)category_mask;
  (void)locale;
  (void)base;
  return (locale_t)(uintptr_t)1;
}

void freelocale(locale_t locale) {
  (void)locale;
}

float strtof_l(const char *str, char **end, locale_t locale) {
  (void)locale;
  return strtof(str, end);
}

int * __errno_location(void) {
  return &errno_slot;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
  char buf[2048];
  int len;
  (void)stream;
  len = vsnprintf(buf, sizeof(buf), fmt, ap);
  shim_write(buf, len);
  return len;
}

int fprintf(FILE *stream, const char *fmt, ...) {
  va_list ap;
  int len;
  va_start(ap, fmt);
  len = vfprintf(stream, fmt, ap);
  va_end(ap);
  return len;
}

int fflush(FILE *stream) {
  (void)stream;
  return 0;
}

void abort(void) {
  halt(1);
  while (1) {}
}

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function) {
  fprintf(stderr, "Assertion failed: %s at %s:%u (%s)\n",
    assertion ? assertion : "?",
    file ? file : "?",
    line,
    function ? function : "?");
  halt(1);
  while (1) {}
}

time_t time(time_t *tloc) {
  AM_TIMER_UPTIME_T uptime;
  time_t now;
  ioe_read(AM_TIMER_UPTIME, &uptime);
  now = (time_t)(946684800ULL + uptime.us / 1000000ULL);
  if (tloc) {
    *tloc = now;
  }
  return now;
}

struct tm *localtime_r(const time_t *timep, struct tm *result) {
  int64_t seconds = *timep;
  int64_t days = seconds / 86400;
  int64_t rem = seconds % 86400;
  int year;
  unsigned month;
  unsigned day;

  if (rem < 0) {
    rem += 86400;
    --days;
  }

  civil_from_days(days, &year, &month, &day);
  result->tm_year = year - 1900;
  result->tm_mon = (int)month - 1;
  result->tm_mday = (int)day;
  result->tm_hour = (int)(rem / 3600);
  rem %= 3600;
  result->tm_min = (int)(rem / 60);
  result->tm_sec = (int)(rem % 60);
  result->tm_wday = (int)((days + 4) % 7);
  if (result->tm_wday < 0) {
    result->tm_wday += 7;
  }
  result->tm_yday = (int)(days - days_from_civil(year, 1, 1));
  result->tm_isdst = 0;
  return result;
}

time_t mktime(struct tm *timeptr) {
  int year = timeptr->tm_year + 1900;
  unsigned month = (unsigned)timeptr->tm_mon + 1;
  unsigned day = (unsigned)timeptr->tm_mday;
  int64_t days = days_from_civil(year, month, day);
  int64_t seconds = days * 86400
    + (int64_t)timeptr->tm_hour * 3600
    + (int64_t)timeptr->tm_min * 60
    + timeptr->tm_sec;
  timeptr->tm_wday = (int)((days + 4) % 7);
  if (timeptr->tm_wday < 0) {
    timeptr->tm_wday += 7;
  }
  timeptr->tm_yday = (int)(days - days_from_civil(year, 1, 1));
  timeptr->tm_isdst = 0;
  return (time_t)seconds;
}

div_t div(int numer, int denom) {
  div_t result;
  result.quot = numer / denom;
  result.rem = numer % denom;
  return result;
}

float sinf(float x) {
  const float pi = 3.14159265358979323846f;
  const float two_pi = 6.28318530717958647692f;
  while (x > pi) {
    x -= two_pi;
  }
  while (x < -pi) {
    x += two_pi;
  }
  {
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f + (x2 * x2) / 120.0f - (x2 * x2 * x2) / 5040.0f);
  }
}

float cosf(float x) {
  return sinf(x + 1.57079632679489661923f);
}

float exp2f(float x) {
  int integer = (int)x;
  float frac = x - integer;
  float frac_pow = 1.0f + 0.69314718056f * frac + 0.24022650695f * frac * frac + 0.05550410866f * frac * frac * frac;
  float scale = 1.0f;
  int i;

  if (frac < 0.0f) {
    integer -= 1;
    frac += 1.0f;
    frac_pow = 1.0f + 0.69314718056f * frac + 0.24022650695f * frac * frac + 0.05550410866f * frac * frac * frac;
  }

  if (integer >= 0) {
    for (i = 0; i < integer; ++i) {
      scale *= 2.0f;
    }
  } else {
    for (i = 0; i < -integer; ++i) {
      scale *= 0.5f;
    }
  }

  return frac_pow * scale;
}

#define CT_UPPER  0x0100
#define CT_LOWER  0x0200
#define CT_ALPHA  0x0400
#define CT_DIGIT  0x0800
#define CT_XDIGIT 0x1000
#define CT_SPACE  0x2000
#define CT_PRINT  0x4000
#define CT_GRAPH  0x8000
#define CT_BLANK  0x0001
#define CT_CNTRL  0x0002
#define CT_PUNCT  0x0004
#define CT_ALNUM  0x0008

static unsigned short ctype_table[384];
static const unsigned short *ctype_ptr = &ctype_table[128];
static bool ctype_ready;

static void init_ctype_table(void) {
  int i;
  if (ctype_ready) {
    return;
  }
  for (i = 0; i < 256; ++i) {
    unsigned short flags = 0;
    if (i < 32 || i == 127) {
      flags |= CT_CNTRL;
    }
    if (ascii_isspace(i)) {
      flags |= CT_SPACE;
    }
    if (i == ' ' || i == '\t') {
      flags |= CT_BLANK;
    }
    if (ascii_isdigit(i)) {
      flags |= CT_DIGIT | CT_ALNUM | CT_PRINT | CT_GRAPH | CT_XDIGIT;
    }
    if (ascii_isalpha(i)) {
      flags |= CT_ALPHA | CT_ALNUM | CT_PRINT | CT_GRAPH;
      if (i >= 'A' && i <= 'Z') {
        flags |= CT_UPPER;
      } else {
        flags |= CT_LOWER;
      }
      if ((i >= 'A' && i <= 'F') || (i >= 'a' && i <= 'f')) {
        flags |= CT_XDIGIT;
      }
    }
    if (i >= 32 && i <= 126) {
      flags |= CT_PRINT;
    }
    if (i >= 33 && i <= 126) {
      flags |= CT_GRAPH;
    }
    if ((i >= 33 && i <= 47) || (i >= 58 && i <= 64) || (i >= 91 && i <= 96) || (i >= 123 && i <= 126)) {
      flags |= CT_PUNCT;
    }
    ctype_table[128 + i] = flags;
  }
  ctype_ready = true;
}

const unsigned short int **__ctype_b_loc(void) {
  init_ctype_table();
  return &ctype_ptr;
}

#endif
