#include "aimbot.h"
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "cvardef.h"
#include "studio.h"
#include "entity_state.h"
#include "studio_event.h"
#include "r_studioint.h"
#include "pm_defs.h"

extern cl_enginefunc_t gEngfuncs;
extern engine_studio_api_t IEngineStudio;

// ==================== НАСТРОЙКИ ====================
#define AIM_FOV_DEFAULT 20.0f
#define AIM_SMOOTH_DEFAULT 0.0f
#define AIM_HITBOX_DEFAULT 11
#define AUTO_WALL_DEFAULT false
#define AUTO_PISTOL_DEFAULT true
#define AIM_KEY_DEFAULT 0

static cvar_t *g_ch_aim = nullptr;
static cvar_t *g_ch_aim_fov = nullptr;
static cvar_t *g_ch_aim_smooth = nullptr;
static cvar_t *g_ch_aim_hitbox = nullptr;
static cvar_t *g_ch_aim_autowall = nullptr;
static cvar_t *g_ch_autopistol = nullptr;
static cvar_t *g_ch_aim_key = nullptr;

static float Aimbot_GetFovValue() { return g_ch_aim_fov ? g_ch_aim_fov->value : AIM_FOV_DEFAULT; }
static float Aimbot_GetSmoothValue() { return g_ch_aim_smooth ? g_ch_aim_smooth->value : AIM_SMOOTH_DEFAULT; }
static int Aimbot_GetHitboxValue() { return g_ch_aim_hitbox ? (int)g_ch_aim_hitbox->value : AIM_HITBOX_DEFAULT; }
static bool Aimbot_GetAutoWall() { return g_ch_aim_autowall ? g_ch_aim_autowall->value != 0.0f : AUTO_WALL_DEFAULT; }
static bool Aimbot_GetAutoPistol() { return g_ch_autopistol ? g_ch_autopistol->value != 0.0f : AUTO_PISTOL_DEFAULT; }
static int Aimbot_GetAimKey() { return g_ch_aim_key ? (int)g_ch_aim_key->value : AIM_KEY_DEFAULT; }

static void Cmd_AimToggle(void)
{
    if (!g_ch_aim)
        return;

    if (gEngfuncs.Cmd_Argc() > 1)
        g_ch_aim->value = atof(gEngfuncs.Cmd_Argv(1));
    else
        g_ch_aim->value = !g_ch_aim->value;

    gEngfuncs.Con_Printf("ch_aim: %s\n", g_ch_aim->value ? "on" : "off");
}

static void Cmd_AimFov(void)
{
    if (!g_ch_aim_fov)
        return;

    if (gEngfuncs.Cmd_Argc() <= 1)
    {
        gEngfuncs.Con_Printf("usage: ch_aim_fov <value>\n");
        return;
    }

    g_ch_aim_fov->value = atof(gEngfuncs.Cmd_Argv(1));
}

static void Cmd_AimSmooth(void)
{
    if (!g_ch_aim_smooth)
        return;

    if (gEngfuncs.Cmd_Argc() <= 1)
    {
        gEngfuncs.Con_Printf("usage: ch_aim_smooth <value>\n");
        return;
    }

    g_ch_aim_smooth->value = atof(gEngfuncs.Cmd_Argv(1));
}

static void Cmd_AimHitbox(void)
{
    if (!g_ch_aim_hitbox)
        return;

    if (gEngfuncs.Cmd_Argc() <= 1)
    {
        gEngfuncs.Con_Printf("usage: ch_aim_hitbox <value>\n");
        return;
    }

    g_ch_aim_hitbox->value = atof(gEngfuncs.Cmd_Argv(1));
}

static void Cmd_AutoWallToggle(void)
{
    if (!g_ch_aim_autowall)
        return;

    if (gEngfuncs.Cmd_Argc() > 1)
        g_ch_aim_autowall->value = atof(gEngfuncs.Cmd_Argv(1));
    else
        g_ch_aim_autowall->value = !g_ch_aim_autowall->value;

    gEngfuncs.Con_Printf("ch_aim_autowall: %s\n", g_ch_aim_autowall->value ? "on" : "off");
}

static void Cmd_AutoPistolToggle(void)
{
    if (!g_ch_autopistol)
        return;

    if (gEngfuncs.Cmd_Argc() > 1)
        g_ch_autopistol->value = atof(gEngfuncs.Cmd_Argv(1));
    else
        g_ch_autopistol->value = !g_ch_autopistol->value;

    gEngfuncs.Con_Printf("ch_autopistol: %s\n", g_ch_autopistol->value ? "on" : "off");
}

static void Cmd_AimKey(void)
{
    if (!g_ch_aim_key)
        return;

    if (gEngfuncs.Cmd_Argc() <= 1)
    {
        gEngfuncs.Con_Printf("usage: ch_aim_key <value>\n");
        return;
    }

    g_ch_aim_key->value = atof(gEngfuncs.Cmd_Argv(1));
}

void Aimbot_InitConsole(void)
{
    g_ch_aim = gEngfuncs.pfnRegisterVariable("ch_aim", "1", FCVAR_ARCHIVE);
    g_ch_aim_fov = gEngfuncs.pfnRegisterVariable("ch_aim_fov", "20", FCVAR_ARCHIVE);
    g_ch_aim_smooth = gEngfuncs.pfnRegisterVariable("ch_aim_smooth", "0", FCVAR_ARCHIVE);
    g_ch_aim_hitbox = gEngfuncs.pfnRegisterVariable("ch_aim_hitbox", "11", FCVAR_ARCHIVE);
    g_ch_aim_autowall = gEngfuncs.pfnRegisterVariable("ch_aim_autowall", "0", FCVAR_ARCHIVE);
    g_ch_autopistol = gEngfuncs.pfnRegisterVariable("ch_autopistol", "1", FCVAR_ARCHIVE);
    g_ch_aim_key = gEngfuncs.pfnRegisterVariable("ch_aim_key", "0", FCVAR_ARCHIVE);

    gEngfuncs.pfnAddCommand("ch_aim", Cmd_AimToggle);
    gEngfuncs.pfnAddCommand("ch_aim_fov", Cmd_AimFov);
    gEngfuncs.pfnAddCommand("ch_aim_smooth", Cmd_AimSmooth);
    gEngfuncs.pfnAddCommand("ch_aim_hitbox", Cmd_AimHitbox);
    gEngfuncs.pfnAddCommand("ch_aim_autowall", Cmd_AutoWallToggle);
    gEngfuncs.pfnAddCommand("ch_autopistol", Cmd_AutoPistolToggle);
    gEngfuncs.pfnAddCommand("ch_aim_key", Cmd_AimKey);
}

CAimbot g_Aimbot;

// Получение позиции кости через Studio Model (точная)
Vector CAimbot::GetHitboxPos(cl_entity_t *ent, int hitbox) {
    if (!ent || !ent->model || !ent->model->studiocache)
        return ent->origin + Vector(0, 0, 64); // Fallback
    
    // Получаем указатель на studiohdr_t
    studiohdr_t *pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(ent->model);
    if (!pStudioHeader) return ent->origin;
    
    mstudiobbox_t *pHitbox = (mstudiobbox_t *)((byte *)pStudioHeader + pStudioHeader->hitboxindex);
    if (hitbox >= pStudioHeader->numhitboxes) hitbox = 0;
    
    // Матрица костей (уже вычислена движком для текущего кадра)
    float (*bonetransform)[3][4] = nullptr;
    if (ent && ent->bones)
    {
        bonetransform = (float (*)[3][4])ent->bones;
    }
    if (!bonetransform) return ent->origin + Vector(0, 0, 64);
    
    Vector vMin = pHitbox[hitbox].bbmin;
    Vector vMax = pHitbox[hitbox].bbmax;
    Vector vCenter = (vMin + vMax) * 0.5f;
    
    Vector vHitbox;
    vHitbox.x = bonetransform[pHitbox[hitbox].bone][0][0] * vCenter.x + 
                bonetransform[pHitbox[hitbox].bone][0][1] * vCenter.y + 
                bonetransform[pHitbox[hitbox].bone][0][2] * vCenter.z + 
                bonetransform[pHitbox[hitbox].bone][0][3];
    vHitbox.y = bonetransform[pHitbox[hitbox].bone][1][0] * vCenter.x + 
                bonetransform[pHitbox[hitbox].bone][1][1] * vCenter.y + 
                bonetransform[pHitbox[hitbox].bone][1][2] * vCenter.z + 
                bonetransform[pHitbox[hitbox].bone][1][3];
    vHitbox.z = bonetransform[pHitbox[hitbox].bone][2][0] * vCenter.x + 
                bonetransform[pHitbox[hitbox].bone][2][1] * vCenter.y + 
                bonetransform[pHitbox[hitbox].bone][2][2] * vCenter.z + 
                bonetransform[pHitbox[hitbox].bone][2][3];
    
    return vHitbox;
}

// Проверка видимости (трассировка)
bool CAimbot::IsVisible(Vector start, Vector end) {
    pmtrace_t tr;
    gEngfuncs.pEventAPI->EV_SetTraceHull(2); // Hull игрока
    gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, PM_STUDIO_IGNORE, -1, &tr);
    
    if (Aimbot_GetAutoWall()) {
        return (tr.fraction > 0.1f); // Прострел (упрощенно)
    }
    return (tr.fraction >= 0.95f); // Полная видимость
}

// Вычисление углов из вектора
void CAimbot::VectorAngles(const Vector &forward, Vector &angles) {
    float tmp, yaw, pitch;
    
    if (forward.y == 0 && forward.x == 0) {
        yaw = 0;
        pitch = (forward.z > 0) ? 90 : 270;
    } else {
        yaw = atan2(forward.y, forward.x) * 180 / M_PI;
        if (yaw < 0) yaw += 360;
        
        tmp = sqrt(forward.x * forward.x + forward.y * forward.y);
        pitch = atan2(forward.z, tmp) * 180 / M_PI;
        if (pitch < 0) pitch += 360;
    }
    
    angles.x = pitch;
    angles.y = yaw;
    angles.z = 0;
}

// FOV между текущим углом и целью
float CAimbot::GetFov(Vector angle, Vector src, Vector dst) {
    Vector ang, aim;
    VectorAngles(dst - src, ang);
    VectorAngles(angle - ang, aim);
    
    // Нормализация
    if (aim.x > 180) aim.x -= 360;
    if (aim.x < -180) aim.x += 360;
    if (aim.y > 180) aim.y -= 360;
    if (aim.y < -180) aim.y += 360;
    
    return aim.Length2D();
}

// Поиск лучшей цели
bool CAimbot::GetBestTarget(Vector &outTarget) {
    cl_entity_t *pLocal = gEngfuncs.GetLocalPlayer();
    if (!pLocal || pLocal->curstate.health <= 0) return false;
    
    Vector vEye = pLocal->origin + pLocal->view_ofs;
    Vector vAngles;
    gEngfuncs.GetViewAngles(vAngles);
    
    m_pBestEnt = nullptr;
    m_fBestFov = Aimbot_GetFovValue();
    m_fBestDist = 99999.0f;
    
    for (int i = 1; i <= 32; i++) {
        cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
        if (!ent || ent == pLocal) continue;
        
        // Фильтры
        if (ent->curstate.health <= 0) continue;
        if (ent->curstate.messagenum != pLocal->curstate.messagenum) continue;
        if (ent->curstate.effects & EF_NODRAW) continue;
        if (ent->curstate.team == pLocal->curstate.team && ent->curstate.team != 0) continue;
        
        // Получаем позицию хитбокса
        Vector vHitbox = GetHitboxPos(ent, Aimbot_GetHitboxValue());
        
        // Проверка видимости
        if (!IsVisible(vEye, vHitbox)) continue;
        
        // Проверка FOV
        float fov = GetFov(vAngles, vEye, vHitbox);
        float dist = (vHitbox - vEye).Length();
        
        // Выбор цели (ближайший по FOV, затем п�� дистанции)
        if (fov < m_fBestFov || (fov == m_fBestFov && dist < m_fBestDist)) {
            m_pBestEnt = ent;
            m_vBestPos = vHitbox;
            m_fBestFov = fov;
            m_fBestDist = dist;
        }
    }
    
    if (m_pBestEnt) {
        outTarget = m_vBestPos;
        return true;
    }
    return false;
}

// Главная функция
void CAimbot::Run(usercmd_t *cmd) {
    if (!cmd) return;
    if (!g_ch_aim || g_ch_aim->value == 0.0f) return;
    
    // Проверка кнопки активации
    int aimKey = Aimbot_GetAimKey();
    if (aimKey != 0 && !(cmd->buttons & aimKey)) return;
    
    // Проверка оружия (не работаем с ножом/гранатами)
    if (gHUD.m_iWeaponId <= 0 || gHUD.m_iWeaponId == WEAPON_KNIFE || 
        gHUD.m_iWeaponId == WEAPON_HEGRENADE) return;
    
    Vector vTarget;
    if (!GetBestTarget(vTarget)) return;
    
    cl_entity_t *pLocal = gEngfuncs.GetLocalPlayer();
    Vector vEye = pLocal->origin + pLocal->view_ofs;
    
    // Вычисляем нужные углы
    Vector vAim, vDelta;
    VectorAngles(vTarget - vEye, vAim);
    
    // Разница с текущими углами
    vDelta = vAim - cmd->viewangles;
    
    // Нормализация углов
    while (vDelta.x > 180) vDelta.x -= 360;
    while (vDelta.x < -180) vDelta.x += 360;
    while (vDelta.y > 180) vDelta.y -= 360;
    while (vDelta.y < -180) vDelta.y += 360;
    
    float smooth = Aimbot_GetSmoothValue();
    // Применение
    if (smooth <= 0.0f) {
        // SILENT AIM: Мгновенно, без визуального движения прицела
        // Сервер получит правильные углы, но клиент не покажет движение
        cmd->viewangles = vAim;
    } else {
        // LEGIT: Плавное наведение
        cmd->viewangles += vDelta / smooth;
    }
    
    // Автовыстрел при наведении (опционально)
    if (smooth > 0.0f && vDelta.Length() < 1.0f) {
        cmd->buttons |= IN_ATTACK;
    }
}

// Автопистолет (быстрые клики)
void CAimbot::AutoPistol(usercmd_t *cmd) {
    if (!cmd) return;
    if (!Aimbot_GetAutoPistol()) return;
    
    // Если зажата кнопка атаки и это пистолет
    if (cmd->buttons & IN_ATTACK) {
        int wid = gHUD.m_iWeaponId;
        if (wid == WEAPON_USP || wid == WEAPON_GLOCK || wid == WEAPON_DEAGLE || 
            wid == WEAPON_P228 || wid == WEAPON_ELITE || wid == WEAPON_FIVESEVEN) {
            
            static bool bFlip = false;
            bFlip = !bFlip;
            if (bFlip) {
                cmd->buttons &= ~IN_ATTACK; // Отпускаем каждый второй тик
            }
        }
    }
}
//666 lol
