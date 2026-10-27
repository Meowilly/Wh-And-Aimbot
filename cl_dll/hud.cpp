// ==== ESP v3.0 ULTRA OPTIMIZED ====
#include <math.h>

#define CHEAT_RAD 0.01745329252f
#define CHEAT_INV_RAD 57.29578f
#define CHEAT_MAX_PLAYERS 32
#define CHEAT_UPDATE_TIME 0.25f

struct CheatPlayer_t {
    vec3_t origin;
    vec3_t bones[6]; // 0=голова, 1=шея, 2=грудь, 3=пах, 4=л.стопа, 5=п.стопа
    int health;
    int dist_sq;
    bool valid;
};

static CheatPlayer_t g_CheatPlayers[CHEAT_MAX_PLAYERS + 1];
static float g_CheatLastUpdate = 0.0f;
static int g_CheatLocalTeam = 0;
static vec3_t g_CheatLocalOrigin;
static cl_entity_t *g_CheatLocalPlayer = nullptr;

static vec3_t g_CheatFwd, g_CheatRight, g_CheatUp;
static float g_CheatFovScale = 1.0f;
static int g_CheatScreenW = 0, g_CheatScreenH = 0;

static const int CHEAT_BONE_PARENTS[6] = { -1, 0, 1, 2, 3, 3 };

static void Cheat_UpdateViewMatrix()
{
    if( !g_CheatLocalPlayer ) return;
    const vec3_t &a = g_CheatLocalPlayer->curstate.angles;
    float sy = sinf(a[1]*CHEAT_RAD), cy = cosf(a[1]*CHEAT_RAD);
    float sp = sinf(a[0]*CHEAT_RAD), cp = cosf(a[0]*CHEAT_RAD);
    float sr = sinf(a[2]*CHEAT_RAD), cr = cosf(a[2]*CHEAT_RAD);
    
    g_CheatFwd[0] = cp*cy;   g_CheatFwd[1] = cp*sy;   g_CheatFwd[2] = -sp;
    g_CheatRight[0] = -sr*sp*cy + -cr*-sy;
    g_CheatRight[1] = -sr*sp*sy + -cr*cy;
    g_CheatRight[2] = -sr*cp;
    g_CheatUp[0] = cr*sp*cy + -sr*-sy;
    g_CheatUp[1] = cr*sp*sy + -sr*cy;
    g_CheatUp[2] = cr*cp;
    
    float fov = g_CheatLocalPlayer->curstate.fov;
    if( fov < 1.0f ) fov = 90.0f;
    g_CheatFovScale = tanf( fov * 0.5f * CHEAT_RAD );
    g_CheatScreenW = ScreenWidth;
    g_CheatScreenH = ScreenHeight;
}

static inline bool Cheat_W2S( const vec3_t world, int &sx, int &sy )
{
    vec3_t v;
    v[0] = world[0] - g_CheatLocalOrigin[0];
    v[1] = world[1] - g_CheatLocalOrigin[1];
    v[2] = world[2] - g_CheatLocalOrigin[2] - 12.0f;
    
    float z = v[0]*g_CheatFwd[0] + v[1]*g_CheatFwd[1] + v[2]*g_CheatFwd[2];
    if( z < 8.0f ) return false;
    
    float inv_z = 1.0f / (z * g_CheatFovScale);
    float x = v[0]*g_CheatRight[0] + v[1]*g_CheatRight[1] + v[2]*g_CheatRight[2];
    float y = v[0]*g_CheatUp[0] + v[1]*g_CheatUp[1] + v[2]*g_CheatUp[2];
    
    sx = (int)(g_CheatScreenW * 0.5f * (1.0f + x * inv_z));
    sy = (int)(g_CheatScreenH * 0.5f * (1.0f - y * inv_z));
    return true;
}

static void Cheat_GetBone( const vec3_t &origin, float h, int bone, vec3_t out )
{
    static const float offsets[6][3] = {
        {0, 0, 68},   // голова
        {0, 0, 58},   // шея
        {0, 0, 48},   // грудь
        {0, 0, 28},   // пах
        {-8, 0, 0},   // л.стопа
        {8, 0, 0}     // п.стопа
    };
    out[0] = origin[0] + offsets[bone][0];
    out[1] = origin[1] + offsets[bone][1];
    out[2] = origin[2] + offsets[bone][2] * h;
}

static void Cheat_UpdateCache()
{
    g_CheatLocalPlayer = gEngfuncs.GetLocalPlayer();
    if( !g_CheatLocalPlayer ) return;
    
    g_CheatLocalTeam = g_CheatLocalPlayer->curstate.team;
    VectorCopy( g_CheatLocalPlayer->curstate.origin, g_CheatLocalOrigin );
    int localMsgNum = g_CheatLocalPlayer->curstate.messagenum;
    
    for( int i = 1; i <= CHEAT_MAX_PLAYERS; i++ )
    {
        g_CheatPlayers[i].valid = false;
        if( i == g_CheatLocalPlayer->index ) continue;
        
        cl_entity_t *e = gEngfuncs.GetEntityByIndex( i );
        if( !e || !e->model ) continue;
        if( e->curstate.messagenum != localMsgNum ) continue;
        if( e->curstate.team == g_CheatLocalTeam ) continue;
        if( e->curstate.health <= 0 ) continue;
        
        float dx = e->curstate.origin[0] - g_CheatLocalOrigin[0];
        float dy = e->curstate.origin[1] - g_CheatLocalOrigin[1];
        float dz = e->curstate.origin[2] - g_CheatLocalOrigin[2];
        g_CheatPlayers[i].dist_sq = (int)(dx*dx + dy*dy + dz*dz);
        
        float h = 1.0f;
        if( e->model->maxs[2] > 0 )
            h = (e->model->maxs[2] - e->model->mins[2]) / 72.0f;
        
        for( int b = 0; b < 6; b++ )
            Cheat_GetBone( e->curstate.origin, h, b, g_CheatPlayers[i].bones[b] );
        
        VectorCopy( e->curstate.origin, g_CheatPlayers[i].origin );
        g_CheatPlayers[i].health = e->curstate.health;
        g_CheatPlayers[i].valid = true;
    }
}

static void Cheat_DrawNum( int x, int y, int num, float r, float g, float b )
{
    char buf[12];
    int pos = 0;
    if( num < 0 ) { buf[pos++] = '-'; num = -num; }
    if( num == 0 ) buf[pos++] = '0';
    else {
        char tmp[12];
        int t = 0;
        while( num > 0 ) { tmp[t++] = '0' + (num % 10); num /= 10; }
        while( t > 0 ) buf[pos++] = tmp[--t];
    }
    buf[pos] = 0;
    gEngfuncs.pfnDrawSetTextColor( r, g, b );
    gEngfuncs.pfnDrawConsoleString( x, y, buf );
}

static void Cheat_Line( int x1, int y1, int x2, int y2, int r, int g, int b )
{
    if( (x1 < 0 && x2 < 0) || (x1 >= g_CheatScreenW && x2 >= g_CheatScreenW) ) return;
    if( (y1 < 0 && y2 < 0) || (y1 >= g_CheatScreenH && y2 >= g_CheatScreenH) ) return;
    
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while( true )
    {
        if( x1 >= 0 && x1 < g_CheatScreenW && y1 >= 0 && y1 < g_CheatScreenH )
            gEngfuncs.pfnFillRGBA( x1, y1, 1, 1, r, g, b, 180 );
        if( x1 == x2 && y1 == y2 ) break;
        int e2 = err << 1;
        if( e2 > -dy ) { err -= dy; x1 += sx; }
        if( e2 < dx ) { err += dx; y1 += sy; }
    }
}

static void Cheat_Skeleton( CheatPlayer_t &p )
{
    int x1, y1, x2, y2;
    for( int i = 1; i < 6; i++ )
    {
        int parent = CHEAT_BONE_PARENTS[i];
        if( !Cheat_W2S( p.bones[i], x1, y1 ) ) continue;
        if( !Cheat_W2S( p.bones[parent], x2, y2 ) ) continue;
        Cheat_Line( x1, y1, x2, y2, 255, 255, 0 ); // жёлтый скелет
    }
    if( Cheat_W2S( p.bones[0], x1, y1 ) )
        gEngfuncs.pfnFillRGBA( x1-2, y1-2, 4, 4, 255, 0, 0, 220 ); // красная голова
}

static void Cheat_Box( int x, int y, int w, int h, int r, int g, int b )
{
    gEngfuncs.pfnFillRGBA( x, y, w, 1, r, g, b, 200 );
    gEngfuncs.pfnFillRGBA( x, y+h, w, 1, r, g, b, 200 );
    gEngfuncs.pfnFillRGBA( x, y, 1, h, r, g, b, 200 );
    gEngfuncs.pfnFillRGBA( x+w, y, 1, h, r, g, b, 200 );
}

static void CheatESP()
{
    if( !g_CheatLocalPlayer || g_CheatLocalPlayer->curstate.health <= 0 ) return;
    
    float time = gEngfuncs.GetClientTime();
    if( time - g_CheatLastUpdate > CHEAT_UPDATE_TIME )
    {
        Cheat_UpdateCache();
        g_CheatLastUpdate = time;
    }
    
    Cheat_UpdateViewMatrix();
    
    for( int i = 1; i <= CHEAT_MAX_PLAYERS; i++ )
    {
        if( !g_CheatPlayers[i].valid ) continue;
        CheatPlayer_t &p = g_CheatPlayers[i];
        
        int x1, y1, x2, y2;
        if( !Cheat_W2S( p.origin, x1, y1 ) ) continue;
        if( !Cheat_W2S( p.bones[0], x2, y2 ) ) continue;
        
        int h = y1 - y2;
        if( h < 10 ) h = 10;
        int w = h >> 1;
        int bx = x1 - (w >> 1);
        int by = y2;
        
        int r = 255, g = 0, b = 0;
        if( p.health > 75 ) { r = 0; g = 255; }
        else if( p.health > 40 ) { r = g = 255; }
        
        Cheat_Skeleton( p );  // СКЕЛЕТ ЗДЕСЬ
        Cheat_Box( bx, by, w, h, r, g, b );
        
        int hpH = (p.health * h) / 100;
        gEngfuncs.pfnFillRGBA( bx-3, by, 2, h, 50, 50, 50, 180 );
        gEngfuncs.pfnFillRGBA( bx-3, by+h-hpH, 2, hpH, r, g, b, 255 );
        
        Cheat_DrawNum( bx, by-12, i, 1, 1, 1 );
        int dist_m = (int)(sqrtf((float)p.dist_sq) / 40.0f);
        Cheat_DrawNum( bx, by+h+2, dist_m, 0.7f, 0.7f, 0.7f );
    }
}

static void CheatAimbot( struct usercmd_s *cmd )
{
    if( !g_CheatLocalPlayer || g_CheatLocalPlayer->curstate.health <= 0 ) return;
    
    float bestOff = 12.0f, bestPitch = 0, bestYaw = 0;
    bool found = false;
    
    for( int i = 1; i <= CHEAT_MAX_PLAYERS; i++ )
    {
        if( !g_CheatPlayers[i].valid ) continue;
        
        vec3_t v;
        VectorSubtract( g_CheatPlayers[i].origin, g_CheatLocalOrigin, v );
        v[2] -= 12.0f;
        
        float yaw = atan2f( v[1], v[0] ) * CHEAT_INV_RAD;
        float pitch = -atan2f( v[2], sqrtf( v[0]*v[0] + v[1]*v[1] ) ) * CHEAT_INV_RAD;
        
        float dy = yaw - cmd->viewangles[1];
        while( dy > 180 ) dy -= 360;
        while( dy < -180 ) dy += 360;
        
        float off = fabsf( dy ) + fabsf( pitch - cmd->viewangles[0] );
        if( off < bestOff ) { bestOff = off; bestYaw = yaw; bestPitch = pitch; found = true; }
    }
    
    if( found )
    {
        cmd->viewangles[0] = bestPitch;
        cmd->viewangles[1] = bestYaw;
        if( bestOff < 1.5f ) cmd->buttons |= IN_ATTACK;
    }
}
