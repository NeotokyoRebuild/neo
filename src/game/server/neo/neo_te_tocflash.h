#ifndef NEO_TE_TOCFLASH_H
#define NEO_TE_TOCFLASH_H

#include "basetempentity.h"

class CNEO_TE_TocFlash : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CNEO_TE_TocFlash, CBaseTempEntity);

	CNEO_TE_TocFlash(const char *name);
	virtual ~CNEO_TE_TocFlash();

	virtual void Test(const Vector &origin, const QAngle &angles);

	DECLARE_SERVERCLASS();

public:
	CNetworkVector(m_vecOrigin);
	CNetworkVar(float, m_fRadius);
	CNetworkVar(int, r);
	CNetworkVar(int, g);
	CNetworkVar(int, b);
	CNetworkVar(int, exponent);
	CNetworkVar(float, m_fTime);
	CNetworkVar(float, m_fDecay);
};

// Singleton
static CNEO_TE_TocFlash g_NEO_TE_TocFlash("TE_TocFlash");

#endif // NEO_TE_TOCFLASH_H