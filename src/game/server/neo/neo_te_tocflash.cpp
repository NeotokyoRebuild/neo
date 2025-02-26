#include "cbase.h"
#include "neo_te_tocflash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CNEO_TE_TocFlash::CNEO_TE_TocFlash(const char *name) :
	CBaseTempEntity(name)
{
	m_vecOrigin.Init();
	r = 0;
	g = 0;
	b = 0;
	exponent = 0;
	m_fRadius = 0.0;
	m_fTime = 0.0;
	m_fDecay = 0.0;
}

CNEO_TE_TocFlash::~CNEO_TE_TocFlash()
{
}

void CNEO_TE_TocFlash::Test(const Vector &origin, const QAngle &angles)
{
	// Fill in data
	r = 255;
	g = 255;
	b = 63;
	m_vecOrigin = origin;

	m_fRadius = 200;
	m_fTime = 2.0;
	m_fDecay = 0.0;

	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors(angles, &forward);
	forward[2] = 0.0;
	VectorNormalize(forward);

	VectorMA(m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify());

	CBroadcastRecipientFilter filter;
	Create(filter, 0.0);
}

IMPLEMENT_SERVERCLASS_ST(CNEO_TE_TocFlash, DT_NEO_TE_TocFlash)
	SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropInt(SENDINFO(r), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(g), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(b), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(exponent), 8, 0),
	SendPropFloat(SENDINFO(m_fRadius), 8, SPROP_ROUNDUP, 0, 2560.0),
	SendPropFloat(SENDINFO(m_fTime), 8, SPROP_ROUNDDOWN, 0, 25.6),
	SendPropFloat(SENDINFO(m_fDecay), 8, SPROP_ROUNDDOWN, 0, 2560.0),
END_SEND_TABLE()

void CNEO_TE_TocFlash(IRecipientFilter &filter, float delay, const Vector* org,
	int r, int g, int b, int exponent, float radius, float time, float decay)
{
	// Set up parameters
	g_NEO_TE_TocFlash.m_vecOrigin = *org;
	g_NEO_TE_TocFlash.r = r;
	g_NEO_TE_TocFlash.g = g;
	g_NEO_TE_TocFlash.b = b;
	g_NEO_TE_TocFlash.exponent = exponent;
	g_NEO_TE_TocFlash.m_fRadius = radius;
	g_NEO_TE_TocFlash.m_fTime = time;
	g_NEO_TE_TocFlash.m_fDecay = decay;

	// Create it
	g_NEO_TE_TocFlash.Create(filter, delay);
}
