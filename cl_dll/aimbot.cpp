#include "aimbot.h"
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "studio_event.h"
#include "r_studioint.h"
#include "pm_defs.h"

extern cl_enginefunc_t gEngfuncs;
extern engine_studio_api_t IEngineStudio;

// ==================== НАСТРОЙКИ ====================
#define AIM_FOV 20.0f           // Угол захвата (градусы)
#define AIM_SMOOTH 0.0f         // 0.0 = Silent (мгновенно), 1.0-10.0 = Legit (плавно)
#define AIM_HITBOX 11           // 11 = Голова, 10 = Грудь, 8 = Живот
#define AUTO_WALL false         // Прострел стен (только через тонкие)
#define AUTO_PISTOL true        // Автострельба для пистолетов
#define AIM_KEY 0               // 0 = всегда активен, или IN_ATTACK, или IN_ALTATTACK

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
    float (*bonetransform)[3][4] = ent->bones;
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
    
    if (AUTO_WALL) {
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
    m_fBestFov = AIM_FOV;
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
        Vector vHitbox = GetHitboxPos(ent, AIM_HITBOX);
        
        // Проверка видимости
        if (!IsVisible(vEye, vHitbox)) continue;
        
        // Проверка FOV
        float fov = GetFov(vAngles, vEye, vHitbox);
        float dist = (vHitbox - vEye).Length();
        
        // Выбор цели (ближайший по FOV, затем по дистанции)
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
    
    // Проверка кнопки активации
    if (AIM_KEY != 0 && !(cmd->buttons & AIM_KEY)) return;
    
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
    
    // Применение
    if (AIM_SMOOTH <= 0.0f) {
        // SILENT AIM: Мгновенно, без визуального движения прицела
        // Сервер получит правильные углы, но клиент не покажет движение
        cmd->viewangles = vAim;
        
        // Для полного Silent нужно также скорректировать punchangle (отдачу)
        // cmd->viewangles.x -= pLocal->punchangle.x * 2;
        // cmd->viewangles.y -= pLocal->punchangle.y * 2;
    } else {
        // LEGIT: Плавное наведение
        cmd->viewangles += vDelta / AIM_SMOOTH;
    }
    
    // Автовыстрел при наведении (опционально)
    if (AIM_SMOOTH > 0 && vDelta.Length() < 1.0f) {
        cmd->buttons |= IN_ATTACK;
    }
}

// Автопистолет (быстрые клики)
void CAimbot::AutoPistol(usercmd_t *cmd) {
    if (!AUTO_PISTOL) return;
    
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
