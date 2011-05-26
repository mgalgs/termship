#ifndef _COMMON_H
#define _COMMON_H

#define MAX_FRAME_WIDTH 200
#define MAX_FILE_LEAF_NAME 200
#define MAX_FILE_FULL_PATH 1000


/* get size of array */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define xstr(s) str(s)
#define str(s) #s


/* TERMSHIP_PATH should *not* have a trailing backslash! */
#ifndef TERMSHIP_PATH
#error "Please define ANIMATIONS_PATH"
#else
#define TERMSHIP_PATH_STR xstr(TERMSHIP_PATH)
#endif

#define KINDLY_DIE_IF_NULL(thing) do {					\
  if ((thing) == NULL) {						\
    kindly_die("Malloc failed! " __FILE__ " " xstr(__LINE__) "\n");	\
  }									\
} while(0)


#endif	/* _COMMON_H */
