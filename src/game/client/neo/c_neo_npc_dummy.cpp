#include "cbase.h"
#include "c_neo_npc_dummy.h"

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

C_NEO_NPCDummy* C_NEO_NPCDummy::GetList()
{
    return dummies.m_pClassList;
}