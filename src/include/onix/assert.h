#ifndef ONIX_ASSERT_H
#define ONIX_ASSERT_H

void assertion_failure(char *exp, char *file, char *base, int line);

#define assert(expr) \
  if (expr);\
  else \
    assertion_failure(#expr, __FILE__, __BASE_FILE__, __LINE__)

void panic(const char *fmt, ...);

#endif