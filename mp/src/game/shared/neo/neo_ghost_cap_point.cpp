#include "cbase.h"
#include "neo_ghost_cap_point.h"

#include "neo_player_shared.h"

#ifdef GAME_DLL
#include "weapon_neobasecombatweapon.h"

#if(0) // wide name localize helpers
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#endif
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NEO NOTE (Rain). These limits defined in original NT FGD as (48 - 256),
// but it seems not even offical maps follow it. I haven't actually checked
// if there's clamping in the original, but setting some sane limits here
// anyways. These are in Hammer units.
#define NEO_CAP_MIN_RADIUS 8
#define NEO_CAP_MAX_RADIUS 10240

#define NEO_FGD_TEAMNUM_ATTACKER 0
#define NEO_FGD_TEAMNUM_DEFENDER 1

LINK_ENTITY_TO_CLASS(neo_ghost_retrieval_point, CNEOGhostCapturePoint);

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST(CNEOGhostCapturePoint, DT_NEOGhostCapturePoint)
	SendPropFloat(SENDINFO(m_flCapzoneRadius)),

	SendPropInt(SENDINFO(m_iOwningTeam)),
	SendPropInt(SENDINFO(m_iSuccessfulCaptorClientIndex)),

	SendPropBool(SENDINFO(m_bGhostHasBeenCaptured)),
	SendPropBool(SENDINFO(m_bIsActive)),
END_SEND_TABLE()
#else
#ifdef CNEOGhostCapturePoint
#undef CNEOGhostCapturePoint
#endif
IMPLEMENT_CLIENTCLASS_DT(C_NEOGhostCapturePoint, DT_NEOGhostCapturePoint, CNEOGhostCapturePoint)
RecvPropFloat(RECVINFO(m_flCapzoneRadius)),

RecvPropInt(RECVINFO(m_iOwningTeam)),
RecvPropInt(RECVINFO(m_iSuccessfulCaptorClientIndex)),

RecvPropBool(RECVINFO(m_bGhostHasBeenCaptured)),
RecvPropBool(RECVINFO(m_bIsActive)),
END_RECV_TABLE()
#define CNEOGhostCapturePoint C_NEOGhostCapturePoint
#endif

BEGIN_DATADESC(CNEOGhostCapturePoint)
#ifdef GAME_DLL
	DEFINE_THINKFUNC(Think_CheckMyRadius),
#endif

// These keyfields come from NT's FGD definition
	DEFINE_KEYFIELD(m_flCapzoneRadius, FIELD_FLOAT, "Radius"),
	DEFINE_KEYFIELD(m_iOwningTeam, FIELD_INTEGER, "team"),
END_DATADESC()

CNEOGhostCapturePoint::CNEOGhostCapturePoint()
{
	m_iSuccessfulCaptorClientIndex = 0;

	m_bIsActive = true;
	m_bGhostHasBeenCaptured = false;

#ifdef CLIENT_DLL
	m_pHUDCapPoint = NULL;
#endif
}

CNEOGhostCapturePoint::~CNEOGhostCapturePoint()
{
#ifdef CLIENT_DLL
	if (m_pHUDCapPoint)
	{
		m_pHUDCapPoint->DeletePanel();
		m_pHUDCapPoint = NULL;
	}
#endif
}

#ifdef GAME_DLL
bool CNEOGhostCapturePoint::IsGhostCaptured(int& outTeamNumber, int& outCaptorClientIndex)
{
	if (m_bIsActive && m_bGhostHasBeenCaptured)
	{
		outTeamNumber = owningTeamAlternate();
		outCaptorClientIndex = m_iSuccessfulCaptorClientIndex;

		return true;
	}

	return false;
}
#endif

int CNEOGhostCapturePoint::owningTeamAlternate() const
{
	const bool alternate = NEORules()->roundAlternate();
	int owningTeam = m_iOwningTeam;
	if (!alternate) owningTeam = (owningTeam == TEAM_JINRAI) ? TEAM_NSF : (owningTeam == TEAM_NSF) ? TEAM_JINRAI : owningTeam;
	return owningTeam;
}

void CNEOGhostCapturePoint::Spawn(void)
{
	BaseClass::Spawn();

	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

#ifdef GAME_DLL
	// This is a Jinrai capzone
	if (m_iOwningTeam == NEO_FGD_TEAMNUM_ATTACKER)
	{
		m_iOwningTeam = TEAM_JINRAI;
	}
	// This is an NSF capzone
	else if (m_iOwningTeam == NEO_FGD_TEAMNUM_DEFENDER)
	{
		m_iOwningTeam = TEAM_NSF;
	}
	else
	{
		// We could recover, but it's probably better to break the capzone
		// and throw a nag message in console so the mapper can fix their error.
		Warning("Capzone had an invalid owning team: %i. Expected %i (Jinrai), or %i (NSF).\n",
			m_iOwningTeam.Get(), NEO_FGD_TEAMNUM_ATTACKER, NEO_FGD_TEAMNUM_DEFENDER);

		// Nobody will be able to cap here.
		m_iOwningTeam = TEAM_INVALID;
	}

	// Warning messages for the about-to-occur clamping, if we've hit limits.
	if (m_flCapzoneRadius < NEO_CAP_MIN_RADIUS)
	{
		Warning("Capzone had too small radius: %f, clamping! (Expected a minimum of %f)\n",
			m_flCapzoneRadius.Get(), NEO_CAP_MIN_RADIUS);
	}
	else if (m_flCapzoneRadius > NEO_CAP_MAX_RADIUS)
	{
		Warning("Capzone had too large radius: %f, clamping! (Expected a minimum of %f)\n",
			m_flCapzoneRadius.Get(), NEO_CAP_MAX_RADIUS);
	}
	// Actually clamp.
	m_flCapzoneRadius = clamp(m_flCapzoneRadius, NEO_CAP_MIN_RADIUS, NEO_CAP_MAX_RADIUS);

	// Set cap zone active if we've got a valid owner.
	SetActive(m_iOwningTeam == TEAM_JINRAI || m_iOwningTeam == TEAM_NSF);

	RegisterThinkContext("CheckMyRadius");
	SetContextThink(&CNEOGhostCapturePoint::Think_CheckMyRadius,
		gpGlobals->curtime, "CheckMyRadius");
#else
	SetNextClientThink(gpGlobals->curtime + NEO_GHOSTCAP_GRAPHICS_THINK_INTERVAL);
#endif
}

#ifdef GAME_DLL
// Purpose: Checks if we have a valid ghoster inside our radius.
void CNEOGhostCapturePoint::Think_CheckMyRadius(void)
{
	if (m_bGhostHasBeenCaptured)
	{
		// We should have been reset after a cap before thinking!
		Assert(false);
		return;
	}

	// This round has already ended, we can't be capped into
	if (NEORules()->GetRoundRemainingTime() < 0)
	{
		return;
	}

	const int checksPerSecond = 10;

	//DevMsg("CNEOGhostCapturePoint::Think_CheckMyRadius\n");

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));

		if (!player)
		{
			continue;
		}

		if (!player->IsCarryingGhost())
		{
			continue;
		}

		const int team = player->GetTeamNumber();

		Assert(team == TEAM_JINRAI || team == TEAM_NSF);
		bool isNotTeamCap = team != owningTeamAlternate();

		// Is this our team's capzone?
		// NEO TODO (Rain): newbie UI helpers for attempting wrong team cap
		if (isNotTeamCap)
		{
			continue;
		}

		const Vector dir = player->GetAbsOrigin() - GetAbsOrigin();
		const float distance = dir.Length2D();

		Assert(distance >= 0);

		// Has the ghost carrier reached inside our radius?
		// NEO TODO (Rain): newbie UI helpers for approaching wrong team cap
		if (distance >= (m_flCapzoneRadius / 2.0f))
		{
			continue;
		}

		// We did it!
		m_bGhostHasBeenCaptured = true;
		m_iSuccessfulCaptorClientIndex = i;

		DevMsg("Player got ghost inside my radius\n");

		player->SendTestMessage("Player captured the ghost!");
		player->m_iCapTeam = player->GetTeamNumber();

		// Center print the cap message.
		// NEO TODO (Rain): better looking message with the team logo.
		CRecipientFilter filter;
		filter.AddAllPlayers();

#if(0) // NEO FIXME (Rain): wide name handling
		wchar wmsg[64 + MAX_PLAYER_NAME_LENGTH];

		wchar wPlayerName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode(player->GetPlayerName(), wPlayerName, sizeof(wPlayerName));

		V_swprintf_safe(wmsg, L"%s wins\n%s captured the ghost",
			player->GetTeamNumber() == TEAM_JINRAI ? L"Jinrai" : L"NSF",
			wPlayerName);
#else
		char msg[64 + MAX_PLACE_NAME_LENGTH];
		COMPILE_TIME_ASSERT(sizeof(msg) <= 512); // max supported

		V_sprintf_safe(msg, "%s wins\n%s captured the ghost",
			player->GetTeamNumber() == TEAM_JINRAI ? "Jinrai" : "NSF",
			player->GetPlayerName());
#endif

		UTIL_ClientPrintFilter(filter, HUD_PRINTCENTER, msg);

		// Return early; we pass next think responsibility to gamerules,
		// whenever it sees fit to start capzone thinking again.
		return;
	}

	SetContextThink(&CNEOGhostCapturePoint::Think_CheckMyRadius,
		gpGlobals->curtime + (1.0f / checksPerSecond), "CheckMyRadius");
}
#else
// Purpose: Set up clientside HUD graphics for capzone.
void CNEOGhostCapturePoint::ClientThink(void)
{
	BaseClass::ClientThink();

	// If we haven't set up capzone HUD graphics yet
	if (!m_pHUDCapPoint)
	{
		m_pHUDCapPoint = new CNEOHud_GhostCapPoint("hudCapZone");
	}

	m_pHUDCapPoint->SetPos(GetAbsOrigin());
	m_pHUDCapPoint->SetRadius(m_flCapzoneRadius);
	m_pHUDCapPoint->SetTeam(m_iOwningTeam); // TODO (nullsystem): Refactor to owningTeamAlternate()

	m_pHUDCapPoint->SetVisible(true);

	SetNextClientThink(gpGlobals->curtime + NEO_GHOSTCAP_GRAPHICS_THINK_INTERVAL);
}
#endif

void CNEOGhostCapturePoint::Precache(void)
{
	BaseClass::Precache();

	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
}

void CNEOGhostCapturePoint::SetActive(bool isActive)
{
	m_bIsActive = isActive;
	UpdateVisibility();
}

inline void CNEOGhostCapturePoint::UpdateVisibility(void)
{
	if (m_bIsActive)
	{
		RemoveEFlags(EF_NODRAW);
	}
	else
	{
		AddEFlags(EF_NODRAW);
	}
}