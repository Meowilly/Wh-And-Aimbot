#pragma once
#include "vector.h"
#include "cl_entity.h"
#include "usercmd.h"

class CAimbot {
public:
    void Run(usercmd_t *cmd);
    void AutoPistol(usercmd_t *cmd);
    
private:
    bool GetBestTarget(Vector &outTarget);
    bool IsVisible(Vector start, Vector end);
    Vector GetHitboxPos(cl_entity_t *ent, int hitbox);
    void VectorAngles(const Vector &forward, Vector &angles);
    float GetFov(Vector angle, Vector src, Vector dst);
    
    cl_entity_t *m_pBestEnt = nullptr;
    Vector m_vBestPos;
    float m_fBestFov;
    float m_fBestDist;
};

extern CAimbot g_Aimbot;
