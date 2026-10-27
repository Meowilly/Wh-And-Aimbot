#if !defined( ENTITY_STATEH )
#define ENTITY_STATEH

#define WHANDAIMBOT_TYPE_FALLBACKS_DEFINED

#include "vector.h"

#ifndef byte
typedef unsigned char byte;
#endif

#ifndef qboolean
typedef int qboolean;
#endif

#ifndef string_t
typedef int string_t;
#endif

#ifndef COLOR24_TYPE_DEFINED
#define COLOR24_TYPE_DEFINED
typedef struct color24_s
{
    byte r;
    byte g;
    byte b;
} color24;
#endif

#ifndef edict_t
struct edict_s;
typedef struct edict_s edict_t;
#endif

#ifndef vec3_t
#define vec3_t Vector
#endif
