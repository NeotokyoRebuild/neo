#ifndef NEO_DETPACK_HITBOX_H
#define NEO_DETPACK_HITBOX_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CNEODeployedDetpack;

class CNEODetpackHitbox : public CBaseEntity
{
public:
	DECLARE_CLASS(CNEODetpackHitbox, CBaseEntity);
	DECLARE_DATADESC();

	void Spawn() override;
	int OnTakeDamage(const CTakeDamageInfo& inputInfo) override;
	bool TestCollision(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr) override;
	bool TestHitboxes(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr) override;

	void SetParentDetpack(CNEODeployedDetpack* pParent);

private:
	CHandle<CNEODeployedDetpack> m_hParentDetpack;
};

#endif // NEO_DETPACK_HITBOX_H