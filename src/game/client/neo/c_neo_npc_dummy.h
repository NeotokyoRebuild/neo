#pragma once

#include "cbase.h"
#include "c_ai_basenpc.h"

class C_NEO_NPCDummy : public C_AI_BaseNPC
{
    DECLARE_CLASS(C_NEO_NPCDummy, C_AI_BaseNPC);
public:
    DECLARE_CLIENTCLASS();

    C_NEO_NPCDummy();
    virtual ~C_NEO_NPCDummy();
    static C_NEO_NPCDummy* GetList();

private:
    int DrawModel(int flags) override;

    friend class C_EntityClassList<C_NEO_NPCDummy>;
    friend class CNEOHud_GhostBeacons;
    C_NEO_NPCDummy* m_pNext = nullptr;
};