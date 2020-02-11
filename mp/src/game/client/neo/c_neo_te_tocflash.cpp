#include "cbase.h"
#include "c_neo_te_tocflash.h"
#include "dlight.h"
#include "iefx.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_NEO_TE_TocFlash, DT_NEO_TE_TocFlash, CNEO_TE_TocFlash)
	RecvPropVector(RECVINFO(m_vecOrigin)),
	RecvPropInt(RECVINFO(r)),
	RecvPropInt(RECVINFO(g)),
	RecvPropInt(RECVINFO(b)),
	RecvPropInt(RECVINFO(exponent)),
	RecvPropFloat(RECVINFO(m_fRadius)),
	RecvPropFloat(RECVINFO(m_fTime)),
	RecvPropFloat(RECVINFO(m_fDecay)),
END_RECV_TABLE()

C_NEO_TE_TocFlash::C_NEO_TE_TocFlash() : C_BaseTempEntity()
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

C_NEO_TE_TocFlash::~C_NEO_TE_TocFlash()
{
}

void TE_TocFlash(IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius,
	float time, float decay, int nLightIndex)
{
	dlight_t *dl = effects->CL_AllocDlight(nLightIndex);
	if (!dl)
	{
		return;
	}

	dl->origin = *org;
	dl->radius = radius;
	dl->color.r = r;
	dl->color.g = g;
	dl->color.b = b;
	dl->color.exponent = exponent;
	dl->die = gpGlobals->curtime + time;
	dl->decay = decay;

	if (ToolsEnabled() && clienttools->IsInRecordingMode())
	{
		Color clr(r, g, b, 255);

		KeyValues *msg = new KeyValues("TempEntity");

		msg->SetInt("te", TE_DYNAMIC_LIGHT);
		msg->SetString("name", "TE_TocFlash");
		msg->SetFloat("time", gpGlobals->curtime);
		msg->SetFloat("duration", time);
		msg->SetFloat("originx", org->x);
		msg->SetFloat("originy", org->y);
		msg->SetFloat("originz", org->z);
		msg->SetFloat("radius", radius);
		msg->SetFloat("decay", decay);
		msg->SetColor("color", clr);
		msg->SetInt("exponent", exponent);
		msg->SetInt("lightindex", nLightIndex);

		ToolFramework_PostToolMessage(HTOOLHANDLE_INVALID, msg);
		msg->deleteThis();
	}
}

void C_NEO_TE_TocFlash::PostDataUpdate(DataUpdateType_t updateType)
{
	VPROF("C_NEO_TE_TocFlash::PostDataUpdate");

	CBroadcastRecipientFilter filter;
	TE_TocFlash(filter, 0.0f, &m_vecOrigin, r, g, b, exponent, m_fRadius, m_fTime, m_fDecay, LIGHT_INDEX_TE_DYNAMIC);
}

void TE_TocFlash(IRecipientFilter& filter, float delay, KeyValues *pKeyValues)
{
	Vector vecOrigin;
	vecOrigin.x = pKeyValues->GetFloat("originx");
	vecOrigin.y = pKeyValues->GetFloat("originy");
	vecOrigin.z = pKeyValues->GetFloat("originz");
	float flDuration = pKeyValues->GetFloat("duration");
	Color c = pKeyValues->GetColor("color");
	int nExponent = pKeyValues->GetInt("exponent");
	float flRadius = pKeyValues->GetFloat("radius");
	float flDecay = pKeyValues->GetFloat("decay");
	int nLightIndex = pKeyValues->GetInt("lightindex", LIGHT_INDEX_TE_DYNAMIC);

	TE_TocFlash(filter, 0.0f, &vecOrigin, c.r(), c.g(), c.b(), nExponent,
		flRadius, flDuration, flDecay, nLightIndex);
}