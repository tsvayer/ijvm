#ifndef UTIL_H
#define UTIL_H

/* debug print */
#ifdef DEBUG

#define log(...) \
    fprintf(stderr,  __VA_ARGS__)
#else
#define log(...)
#endif

#endif
