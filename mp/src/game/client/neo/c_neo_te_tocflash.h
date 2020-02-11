#ifndef NEO_TE_TOCFLASH_H
#define NEO_TE_TOCFLASH_H

#include "c_basetempentity.h"

class C_NEO_TE_TocFlash : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_NEO_TE_TocFlash, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	C_NEO_TE_TocFlash();
	virtual ~C_NEO_TE_TocFlash();

	virtual void	PostDataUpdate(DataUpdateType_t updateType);

public:
	Vector			m_vecOrigin;
	float			m_fRadius;
	int				r;
	int				g;
	int				b;
	int				exponent;
	float			m_fTime;
	float			m_fDecay;
};

#endif // NEO_TE_TOCFLASH_H