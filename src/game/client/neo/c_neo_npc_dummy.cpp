#include "cbase.h"
#include "c_neo_npc_dummy.h"
#include "c_ai_basenpc.h"
#include "model_types.h"
#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_NEO_NPCDummy, DT_NEO_NPCDummy, CNEO_NPCDummy)
END_RECV_TABLE()

static C_EntityClassList<C_NEO_NPCDummy> dummies;
template <> C_NEO_NPCDummy* C_EntityClassList<C_NEO_NPCDummy>::m_pClassList = nullptr;

C_NEO_NPCDummy::C_NEO_NPCDummy()
{
    dummies.Insert(this);
}

C_NEO_NPCDummy::~C_NEO_NPCDummy()
{
    dummies.Remove(this);
}

int C_NEO_NPCDummy::DrawModel(int flags) // From c_neo_player
{
    if (flags & STUDIO_IGNORE_NEO_EFFECTS || !(flags & STUDIO_RENDER))
    {
        return BaseClass::DrawModel(flags);
    }

    auto pTargetPlayer = C_NEO_Player::GetVisionTargetNEOPlayer();
    if (!pTargetPlayer)
    {
        Assert(false);
        return BaseClass::DrawModel(flags);
    }

    bool inThermalVision = pTargetPlayer ? (pTargetPlayer->IsInVision() && pTargetPlayer->GetClass() == NEO_CLASS_SUPPORT) : false;

    int ret = 0;
    if (inThermalVision)
    {
        IMaterial* pass = materials->FindMaterial("dev/thermal_model", TEXTURE_GROUP_MODEL);
        modelrender->ForcedMaterialOverride(pass);
        ret = BaseClass::DrawModel(flags);
        modelrender->ForcedMaterialOverride(nullptr);
    }
    else
    {
        ret = BaseClass::DrawModel(flags);
    }

    return ret;
}

C_NEO_NPCDummy* C_NEO_NPCDummy::GetList()
{
    return dummies.m_pClassList;
}