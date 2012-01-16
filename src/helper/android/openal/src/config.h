#ifndef CONFIG_H
#define CONFIG_H

/* Define to the library version */
#define ALSOFT_VERSION "1.0.0"

/* Define if we have the PulseAudio backend */
#define HAVE_ANDROID 1

/* Define if we have dlfcn.h */
#define HAVE_DLFCN_H 1

/* Define if we have the stat function */
#define HAVE_STAT 1

/* Define if we have the powf function */
#define HAVE_POWF

/* Define if we have the sqrtf function */
#define HAVE_SQRTF

/* Define if we have the acosf function */
#define HAVE_ACOSF

/* Define if we have the atanf function */
#define HAVE_ATANF

/* Define if we have the fabsf function */
#define HAVE_FABSF

/* Define if we have the strtof function */
#define HAVE_STRTOF

/* Define if we have stdint.h */
#define HAVE_STDINT_H

#define HAVE_NANOSLEEP

#define HAVE_TIME_H

/* Define if we have the __int64 type */
#define HAVE___INT64

/* Define to the size of a long int type */
#define SIZEOF_LONG 4

/* Define to the size of a long long int type */
#define SIZEOF_LONG_LONG 8

/* Define to the size of an unsigned int type */
#define SIZEOF_UINT 4

/* Define to the size of a void pointer type */
#define SIZEOF_VOIDP 4

/* Define if we have GCC's destructor attribute */
#define HAVE_GCC_DESTRUCTOR 1

/* Define if we have GCC's format attribute */
#define HAVE_GCC_FORMAT 1

/* Define if we have pthread_np.h */
/*
#define HAVE_PTHREAD_NP_H
*/

/* Define if we have float.h */
/*
#define HAVE_FLOAT_H
*/

/* Define if we have fenv.h */
#define HAVE_FENV_H 1

/* Define if we have fesetround() */
#define HAVE_FESETROUND 1

/* Define if we have _controlfp() */
/*
#define HAVE__CONTROLFP
*/

/* Define if we have pthread_setschedparam() */
#define HAVE_PTHREAD_SETSCHEDPARAM 1

#endif
