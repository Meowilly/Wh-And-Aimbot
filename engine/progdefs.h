#ifndef PROGDEFS_H
#define PROGDEFS_H

#define BOUNCE_METAL		BREAK_METAL
#define BOUNCE_FLESH		BREAK_FLESH
#define BOUNCE_WOOD			BREAK_WOOD
#define BOUNCE_SHRAP		0x10
#define BOUNCE_SHELL		0x20
#define BOUNCE_CONCRETE		BREAK_CONCRETE
#define BOUNCE_SHOTSHELL		0x80

// Temp entity bounce sound types
#define TE_BOUNCE_NULL		0
#define TE_BOUNCE_SHELL		1
#define TE_BOUNCE_SHOTSHELL	2

// Rendering constants
enum 
{ 	
	kRenderNormal,		// src
	kRenderTransColor,		// c*a+dest*(1-a)
	kRenderTransTexture,	// src*a+dest*(1-a)
	kRenderGlow,		// src*a+dest -- No Z buffer checks
	kRenderTransAlpha,		// src*srca+dest*(1-srca)
	kRenderTransAdd,		// src*a+dest
	kRenderWorldGlow		// Same as kRenderGlow but not fixed size in screen space
};

enum 
{	
	kRenderFxNone = 0, 
	kRenderFxPulseSlow, 
	kRenderFxPulseFast, 
	kRenderFxPulseSlowWide, 
	kRenderFxPulseFastWide, 
	kRenderFxFadeSlow, 
	kRenderFxFadeFast, 
	kRenderFxSolidSlow, 
	kRenderFxSolidFast,	   
	kRenderFxStrobeSlow, 
	kRenderFxStrobeFast, 
	kRenderFxStrobeFaster, 
	kRenderFxFlickerSlow, 
	kRenderFxFlickerFast,
	kRenderFxNoDissipation,
	kRenderFxDistort,			// Distort/scale/translate flicker
	kRenderFxHologram,			// kRenderFxDistort + distance fade
	kRenderFxDeadPlayer,		// kRenderAmt is the player index
	kRenderFxExplode,			// Scale up really big!
	kRenderFxGlowShell,			// Glowing Shell
	kRenderFxClampMinScale		// Keep this sprite from getting very small (SPRITES only!)
};

typedef int		func_t;
typedef int		string_t;

typedef unsigned char	byte;
typedef unsigned short	word;

#undef true
#undef false

#ifndef __cplusplus
typedef enum { false, true }	qboolean;
#else 
typedef int qboolean;
#endif

#ifndef COLOR24_TYPE_DEFINED
#define COLOR24_TYPE_DEFINED
typedef struct
{
	byte	r, g, b;
} color24;
#endif

typedef struct
{
	unsigned	r, g, b, a;
} colorVec;

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

typedef struct edict_s edict_t;

// The shared engine/DLL entity layout is defined in common/entity_state.h.
// Some code paths include this file directly, so make sure that type is always available
// before any edict_t-contained members are declared.
#include "../common/entity_state.h"

typedef struct
{
	vec3_t	normal;
	float	dist;
} plane_t;

typedef struct
{
	qboolean	allsolid;		// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	qboolean	inopen, inwater;
	float		fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	plane_t		plane;		// surface normal at impact
	edict_t		*ent;		// entity the surface is on
	int			hitgroup;		// 0 == generic, non zero is specific body part
} trace_t;

#endif // PROGDEFS_H
