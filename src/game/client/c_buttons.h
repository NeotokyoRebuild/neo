#pragma once

#include "c_baseentity.h"

class C_BaseButton : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_BaseButton, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	int m_spawnflags;
	char m_szUseHintText[64];
	bool m_bUseHint;
	
	virtual int	ObjectCaps(void) override;
};