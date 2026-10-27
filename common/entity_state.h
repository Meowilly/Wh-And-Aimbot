#ifndef ENTITY_STATEH
#define ENTITY_STATEH
#ifdef _WIN32
#ifndef __MINGW32__
#pragma once
#endif /* not __MINGW32__ */
#endif

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

#ifndef edict_t
struct edict_s;
typedef struct edict_s edict_t;
#endif

#ifndef vec3_t
#define vec3_t Vector
#endif

typedef struct
{	
	float		time;
	float		frametime;
	float		force_retouch;
	string_t	mapname;
	string_t	startspot;
	float		deathmatch;
	float		coop;
	float		teamplay;
	float		serverflags;
	float		found_secrets;
	vec3_t		v_forward;
	vec3_t		v_up;
	vec3_t		v_right;
	float		trace_allsolid;
	float		trace_startsolid;
	float		trace_fraction;
	vec3_t		trace_endpos;
	vec3_t		trace_plane_normal;
	float		trace_plane_dist;
	edict_t		*trace_ent;
	float		trace_inopen;
	float		trace_inwater;
	int			trace_hitgroup;
	int			trace_flags;
	int			msg_entity;
	int			cdAudioTrack;
	int			maxClients;
	int			maxEntities;
	const char	*pStringBase;

	void		*pSaveData;
	vec3_t		vecLandmarkOffset;
} globalvars_t;


typedef struct entvars_s
{
	string_t	classname;
	string_t	globalname;

	vec3_t		origin;
	vec3_t		oldorigin;
	vec3_t		velocity;
	vec3_t		basevelocity;
	vec3_t	  clbasevelocity;
	vec3_t		movedir;

	vec3_t		angles;
	vec3_t		avelocity;
	vec3_t		punchangle;
	vec3_t		v_angle;

	vec3_t		endpos;
	vec3_t		startpos;
	float		impacttime;
	float		starttime;

	int		fixangle;
	float		idealpitch;
	float		pitch_speed;
	float		ideal_yaw;
	float		yaw_speed;

	int		modelindex;
	string_t	model;

	int		viewmodel;
	int		weaponmodel;
	
	vec3_t		absmin;
	vec3_t		absmax;
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		size;

	float		ltime;
	float		nextthink;

	int		movetype;
	int		solid;

	int		skin;
	int		body;
	int 		effects;
	
	float		gravity;
	float		friction;
	
	int		light_level;

	int		sequence;
	int		gaitsequence;
	float		frame;
	float		animtime;
	float		framerate;
	float		scale;

	int		rendermode;
	float		renderamt;
	vec3_t		rendercolor;
	int		renderfx;

	float		health;
	float		frags;
	int		weapons;
	float		takedamage;

	int		deadflag;
	vec3_t		view_ofs;

	int		button;
	int		impulse;

	edict_t		*chain;
	edict_t		*dmg_inflictor;
	edict_t		*enemy;
	edict_t		*aiment;
	edict_t		*owner;
	edict_t		*groundentity;

	int		spawnflags;
	int		flags;
	
	int		colormap;
	int		team;

	float		max_health;
	float		teleport_time;
	float		armortype;
	float		armorvalue;
	int		waterlevel;
	int		watertype;
	
	string_t	target;
	string_t	targetname;
	string_t	netname;
	string_t	message;

	float		dmg_take;
	float		dmg_save;
	float		dmg;
	float		dmgtime;
	
	string_t	noise;
	string_t	noise1;
	string_t	noise2;
	string_t	noise3;
	
	float		speed;
	float		air_finished;
	float		pain_finished;
	float		radsuit_finished;
	
	edict_t		*pContainingEntity;

	int		playerclass;
	float		maxspeed;

	float		fov;
	int		weaponanim;

	int		pushmsec;

	int		bInDuck;
	int		flTimeStepSound;
	int		flSwimTime;
	int		flDuckTime;
	int		iStepLeft;
	float		flFallVelocity;

	int		gamestate;

	int		oldbuttons;

	int		groupinfo;

	int		iuser1;
	int		iuser2;
	int		iuser3;
	int		iuser4;
	float		fuser1;
	float		fuser2;
	float		fuser3;
	float		fuser4;
	vec3_t		vuser1;
	vec3_t		vuser2;
	vec3_t		vuser3;
	vec3_t		vuser4;
	edict_t		*euser1;
	edict_t		*euser2;
	edict_t		*euser3;
	edict_t		*euser4;
} entvars_t;

#endif // ENTITY_STATEH
