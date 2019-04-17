/*

  hash_unnest.h --

  Helper macros

*/

#ifndef __HASH_UNNEST_H__
#define __HASH_UNNEST_H__ 1

#ifdef DEBUG
  #define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
  #define LOG(...)
#endif

#endif
