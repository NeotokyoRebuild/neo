#pragma once

#include "cbase.h"
#include "ai_basenpc.h"

class CNEO_NPCDummy : public CAI_BaseNPC {
	DECLARE_CLASS(CNEO_NPCDummy, CAI_BaseNPC);
	DECLARE_SERVERCLASS();

public:
	CNEO_NPCDummy();
	virtual ~CNEO_NPCDummy();
	static CNEO_NPCDummy* GetList();

	virtual void			Precache(void) override;
	virtual void			Spawn(void) override;
	virtual Class_T			Classify(void) override;
	virtual int				ShouldTransmit(const CCheckTransmitInfo* pInfo) override;
	virtual int				UpdateTransmitState() override;

	DECLARE_DATADESC();

	DEFINE_CUSTOM_AI;

private:
	void WeaponBonemerge(const char* weaponModelDir);

	int m_iKeyHealth;
	string_t m_strKeyModel;
	string_t m_strKeyWeaponModel;
	bool m_bKeyDistortEffect;

	friend class CEntityClassList<CNEO_NPCDummy>;
	friend class CNEORules;
	friend class CWeaponGhost;
	CNEO_NPCDummy* m_pNext = nullptr;
};