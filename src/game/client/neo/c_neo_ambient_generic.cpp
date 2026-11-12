#include "cbase.h"
#include "c_neo_ambient_generic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_AmbientGeneric, DT_AmbientGeneric, CAmbientGeneric)
	RecvPropInt(RECVINFO(m_nNetSoundSourceIndex)),
	RecvPropInt(RECVINFO(m_nNetSoundLevel)),
	RecvPropString(RECVINFO(m_szNetSoundFile)),
END_RECV_TABLE()

CUtlVector<C_AmbientGeneric *> C_AmbientGeneric::s_ambients;

C_AmbientGeneric::C_AmbientGeneric()
{
	s_ambients.AddToTail(this);
}

C_AmbientGeneric::~C_AmbientGeneric()
{
	s_ambients.FindAndFastRemove(this);
}
