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

#ifdef CLIENT_DLL
#include "ui/neo_hud_ghost_cap_point.h"
#include "materialsystem/imaterialsystem.h"
#include "cliententitylist.h"
#include "neo_smoke_visual_state.h"
#include <math.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// In seconds, how often should the client think to update capzone graphics info.
// This is *not* the framerate, only info like which team it belongs to, etc.
// Actual rendering happens in the element's Paint() loop.
static constexpr int NEO_GHOSTCAP_GRAPHICS_THINK_INTERVAL = 1;

// NEO NOTE (Rain). These limits defined in original NT FGD as (48 - 256),
// but it seems not even offical maps follow it. I haven't actually checked
// if there's clamping in the original, but setting some sane limits here
// anyways. These are in Hammer units.
static constexpr float NEO_CAP_MIN_RADIUS = 8.0f;
static constexpr float NEO_CAP_MAX_RADIUS = 10240.0f;

static constexpr int NEO_FGD_TEAMNUM_ATTACKER = 0;
static constexpr int NEO_FGD_TEAMNUM_DEFENDER = 1;
static constexpr int NEO_FGD_TEAMNUM_NEUTRAL = 2;

LINK_ENTITY_TO_CLASS(neo_ghost_retrieval_point, CNEOGhostCapturePoint);

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST(CNEOGhostCapturePoint, DT_NEOGhostCapturePoint)
	SendPropFloat(SENDINFO(m_flCapzoneRadius)),
	SendPropInt(SENDINFO(m_iOwningTeam)),
	SendPropBool(SENDINFO(m_bIsActive)),
END_SEND_TABLE()
#else
#ifdef CNEOGhostCapturePoint
#undef CNEOGhostCapturePoint
#endif
IMPLEMENT_CLIENTCLASS_DT(C_NEOGhostCapturePoint, DT_NEOGhostCapturePoint, CNEOGhostCapturePoint)
	RecvPropFloat(RECVINFO(m_flCapzoneRadius)),
	RecvPropInt(RECVINFO(m_iOwningTeam)),
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

#ifdef GAME_DLL
// These are new
	DEFINE_KEYFIELD(m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled"),

// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

// Outputs
	DEFINE_OUTPUT(m_OnCap, "OnCap"),
#endif
END_DATADESC()

enum NeoCapEdgeType
{
	NEO_CAP_EDGE_OFF = 0, 		// disabled
	NEO_CAP_EDGE_LINE,			// show only a line around the capzone
	NEO_CAP_EDGE_LINE_LOGO,		// show the line + a team logo
};

#ifdef CLIENT_DLL
ConVar cl_neo_cap_zone_edge("cl_neo_cap_zone_edge", "2", FCVAR_ARCHIVE, "Cap zone edge rendering", true, 0, true, NEO_CAP_EDGE_LINE_LOGO);

// The ring is a translucent renderable drawn in the engine's last translucent
// pass (IgnoresZBuffer()), after all smoke has been blended. Smoke writes no
// depth, so to hide the ring behind it we lay down an invisible, depth-only
// sphere per nearby smoke cloud right before drawing the ring and let the ring's
// normal depth test do the rest. See NeoCapRing_DrawSmokeProxies().
ConVar cl_neo_cap_ring_smoke_proxy("cl_neo_cap_ring_smoke_proxy", "1", FCVAR_DEVELOPMENTONLY,
	"Hide cap zone ring segments behind nearby smoke using invisible depth-only proxy spheres.");
// The proxy follows the smoke's real expanding sphere (radius 0 -> ~330 over
// SMOKESPHERE_EXPAND_TIME). The visibly opaque core is smaller than that radius
// (density falls off towards the edge), so this scales it down.
ConVar cl_neo_cap_ring_smoke_proxy_scale("cl_neo_cap_ring_smoke_proxy_scale", "0.575", FCVAR_DEVELOPMENTONLY,
	"Smoke proxy sphere radius as a fraction of the smoke's current expanding radius.", true, 0.05f, true, 1.5f);
// While the smoke dissipates its alpha fades 1 -> 0; the proxy radius is
// multiplied by fade^power so it shrinks (and the ring reappears) sooner than
// the smoke visually thins out. 1 = shrinks in step with the fade.
ConVar cl_neo_cap_ring_smoke_proxy_fade_power("cl_neo_cap_ring_smoke_proxy_fade_power", "0", FCVAR_DEVELOPMENTONLY,
	"How much faster the smoke proxy shrinks than the smoke fades while dissipating.", true, 0.0f, true, 8.0f);
ConVar cl_neo_cap_ring_smoke_proxy_debug("cl_neo_cap_ring_smoke_proxy_debug", "0", FCVAR_DEVELOPMENTONLY,
	"Draw the smoke proxy spheres as opaque magenta (instead of depth-only) for tuning.");
#endif

CNEOGhostCapturePoint::CNEOGhostCapturePoint()
{
	m_flCapzoneRadius = -1;
	m_iOwningTeam = TEAM_INVALID;
	m_bIsActive = true;

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
int CNEOGhostCapturePoint::UpdateTransmitState()
{
	return FL_EDICT_ALWAYS;
}

bool CNEOGhostCapturePoint::IsGhostCaptured(int& outTeamNumber, int& outCaptorClientIndex)
{
	if (m_bIsActive && m_bGhostHasBeenCaptured)
	{
		outCaptorClientIndex = m_iSuccessfulCaptorClientIndex;
		
		CBaseEntity* pCaptor = UTIL_PlayerByIndex(m_iSuccessfulCaptorClientIndex);
		if (!pCaptor) // The capzone will be the activator if we can't find the guy who capped it
		{
			pCaptor = this;
			outTeamNumber = owningTeamAlternate();
		}
		else
		{
			outTeamNumber = pCaptor->GetTeamNumber();
		}
		m_OnCap.FireOutput(pCaptor, this);

		return true;
	}

	return false;
}
#endif

int CNEOGhostCapturePoint::owningTeamAlternate() const
{
	const bool alternate = NEORules()->roundNumberIsEven();
	int owningTeam = m_iOwningTeam;
	if (!alternate && owningTeam != TEAM_ANY)
	{
		owningTeam = (owningTeam == TEAM_JINRAI) ? TEAM_NSF : (owningTeam == TEAM_NSF) ? TEAM_JINRAI : owningTeam;
	}
	return owningTeam;
}

void CNEOGhostCapturePoint::Spawn(void)
{
	Precache();
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
	else if (m_iOwningTeam == NEO_FGD_TEAMNUM_NEUTRAL)
	{
		m_iOwningTeam = TEAM_ANY;
	}
	else
	{
		// We could recover, but it's probably better to break the capzone
		// and throw a nag message in console so the mapper can fix their error.
		Warning("Capzone at position %.1f %.1f %.1f had an invalid owning team: %i. "
			"Expected %i (Jinrai), %i (NSF), or %i (neutral).\n",
			GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z,
			m_iOwningTeam.Get(), NEO_FGD_TEAMNUM_ATTACKER, NEO_FGD_TEAMNUM_DEFENDER, NEO_FGD_TEAMNUM_NEUTRAL);

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

	if (!m_bStartDisabled)
	{
		// Set cap zone active if we've got a valid owner.
		SetActive(m_iOwningTeam == TEAM_JINRAI || m_iOwningTeam == TEAM_NSF || m_iOwningTeam == TEAM_ANY);
	}
	else
	{
		SetActive(false);
	}

	RegisterThinkContext("CheckMyRadius");
	SetContextThink(&CNEOGhostCapturePoint::Think_CheckMyRadius,
		gpGlobals->curtime, "CheckMyRadius");
#else
	m_pRingMaterial = materials->FindMaterial("effects/cap_zone_ring", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_pRingNsfMaterial = materials->FindMaterial("effects/cap_zone_ring_nsf", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_pRingJinraiMaterial = materials->FindMaterial("effects/cap_zone_ring_jinrai", TEXTURE_GROUP_CLIENT_EFFECTS);
	AddToLeafSystem(RENDER_GROUP_TRANSLUCENT_ENTITY);
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
	if (NEORules()->IsRoundOver())
	{
		return;
	}

	const int checksPerSecond = 10;

	//DevMsg("CNEOGhostCapturePoint::Think_CheckMyRadius\n");
	if (NEORules()->IsRoundLive() && m_bIsActive)
	{ // VIP can escort themselves if sitting in their extract as the round restarts
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CNEO_Player *player = static_cast<CNEO_Player*>(UTIL_EntityByIndex(i));

			if (!player)
			{
				continue;
			}

			if (player->IsCarryingGhost() || player->GetClass() == NEO_CLASS_VIP)
			{
				const int team = player->GetTeamNumber();

				Assert(team == TEAM_JINRAI || team == TEAM_NSF);
				bool isNotTeamCap = (m_iOwningTeam == TEAM_ANY) ? false : (team != owningTeamAlternate());

				// Is this our team's capzone?
				// NEO TODO (Rain): newbie UI helpers for attempting wrong team cap
				if (isNotTeamCap)
				{
					continue;
				}

				const Vector dir = player->GetAbsOrigin() - GetAbsOrigin();
				const int distance = static_cast<int>(dir.Length());

				Assert(distance >= 0);

				// Has the ghost carrier reached inside our radius?
				// NEO TODO (Rain): newbie UI helpers for approaching wrong team cap
				if (distance > m_flCapzoneRadius)
				{
					continue;
				}

				// We did it!
				m_bGhostHasBeenCaptured = true;
				m_iSuccessfulCaptorClientIndex = i;

				DevMsg("Player got ghost inside my radius\n");

				// Return early; we pass next think responsibility to gamerules,
				// whenever it sees fit to start capzone thinking again.
				return;
			}
		}
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
	m_pHUDCapPoint->SetTeam(owningTeamAlternate());
	m_pHUDCapPoint->SetVisible(m_bIsActive);

	SetNextClientThink(gpGlobals->curtime + NEO_GHOSTCAP_GRAPHICS_THINK_INTERVAL);
}

bool CNEOGhostCapturePoint::ShouldDraw()
{
	return m_bIsActive;
}

RenderGroup_t CNEOGhostCapturePoint::GetRenderGroup()
{
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}

void CNEOGhostCapturePoint::GetRenderBoundsWorldspace(Vector& mins, Vector& maxs)
{
	const Vector& origin = GetAbsOrigin();
	const float r = m_flCapzoneRadius;
	mins = origin + Vector(-r, -r, -16.0f);
	maxs = origin + Vector(r, r, 16.0f);
}

struct NeoCapSmokeProxy_t
{
	Vector center;
	float radius;
};

static constexpr float SMOKE_PROXY_MIN_RADIUS = 8.0f;
static constexpr int SMOKE_PROXY_MAX_COUNT = 16;

// Finds active smoke grenades whose proxy sphere overlaps this cap zone's
// circle in the XY plane. The proxy follows the smoke's real state: it grows
// with the expanding sphere after detonation and shrinks faster than the smoke
// fades while it dissipates, so the ring doesn't pop out/in.
static void NeoCapRing_FindSmokeProxies(const Vector& capOrigin, float capRadius, CUtlVector<NeoCapSmokeProxy_t>& outProxies)
{
	const float scale = cl_neo_cap_ring_smoke_proxy_scale.GetFloat();
	const float fadePower = cl_neo_cap_ring_smoke_proxy_fade_power.GetFloat();
	const Vector2D capOrigin2D = capOrigin.AsVector2D();

	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	while ((pEnt = iterator.Next()) != NULL && outProxies.Count() < SMOKE_PROXY_MAX_COUNT)
	{
		IClientNetworkable *pNet = pEnt->GetClientNetworkable();
		if (!pNet || !pNet->GetClientClass() ||
			Q_stricmp(pNet->GetClientClass()->m_pNetworkName, "ParticleSmokeGrenade") != 0)
		{
			continue;
		}

		NeoSmokeGrenadeVisualState_t state;
		if (!NeoGetSmokeGrenadeVisualState(pEnt, state))
		{
			continue;
		}

		const float radius = state.expandRadius * scale * powf(state.fade, fadePower);
		if (radius < SMOKE_PROXY_MIN_RADIUS)
		{
			continue;
		}

		// The ring is a low, flat circle but the proxy extends as far in Z as in
		// XY, so test overlap in the XY plane only.
		if (capOrigin2D.DistTo(state.center.AsVector2D()) > capRadius + radius)
		{
			continue;
		}

		NeoCapSmokeProxy_t& proxy = outProxies[outProxies.AddToTail()];
		proxy.center = state.center;
		proxy.radius = radius;
	}
}

// Draws one UV sphere per proxy. Each quad is emitted with both windings so
// the result doesn't depend on the bound material's cull mode (otherwise only
// the far hemisphere might write depth and the ring would show through).
static void NeoCapRing_DrawSmokeProxies(const CUtlVector<NeoCapSmokeProxy_t>& proxies)
{
	// The sphere is split into LATITUDE_BANDS rings from pole to pole, each ring
	// into LONGITUDE_SEGMENTS quads around the vertical axis.
	constexpr int LATITUDE_BANDS = 8;
	constexpr int LONGITUDE_SEGMENTS = 12;

	const bool debug = cl_neo_cap_ring_smoke_proxy_debug.GetBool();

	IMaterial *pMaterial = materials->FindMaterial("debug/debugvertexcolor", TEXTURE_GROUP_OTHER);
	if (!pMaterial || pMaterial->IsErrorMaterial())
	{
		return;
	}

	CMatRenderContextPtr pRenderContext(materials);
	if (!debug)
	{
		// Depth only: the proxies must stay invisible.
		pRenderContext->OverrideColorWriteEnable(true, false);
		pRenderContext->OverrideAlphaWriteEnable(true, false);
	}

	pRenderContext->Bind(pMaterial);
	IMesh *pMesh = pRenderContext->GetDynamicMesh(true);
	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, proxies.Count() * LATITUDE_BANDS * LONGITUDE_SEGMENTS * 2);

	auto emitSphereVertex = [&](const Vector& sphereCenter, float sphereRadius, int latitudeLine, int longitudeLine)
	{
		const float polarAngle = M_PI_F * ((float)latitudeLine / LATITUDE_BANDS);
		const float azimuthAngle = 2.0f * M_PI_F * ((float)longitudeLine / LONGITUDE_SEGMENTS);

		const Vector surfaceNormal(
			sinf(polarAngle) * cosf(azimuthAngle),
			sinf(polarAngle) * sinf(azimuthAngle),
			cosf(polarAngle));
		const Vector surfacePosition = sphereCenter + surfaceNormal * sphereRadius;

		meshBuilder.Position3f(surfacePosition.x, surfacePosition.y, surfacePosition.z);
		meshBuilder.Normal3f(surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);
		meshBuilder.Color4ub(255, 0, 255, 255);
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();
	};

	FOR_EACH_VEC(proxies, proxyIndex)
	{
		const Vector& sphereCenter = proxies[proxyIndex].center;
		const float sphereRadius = proxies[proxyIndex].radius;

		for (int band = 0; band < LATITUDE_BANDS; band++)
		{
			for (int segment = 0; segment < LONGITUDE_SEGMENTS; segment++)
			{
				const int northLine = band;
				const int southLine = band + 1;
				const int westLine = segment;
				const int eastLine = segment + 1;

				emitSphereVertex(sphereCenter, sphereRadius, northLine, westLine);
				emitSphereVertex(sphereCenter, sphereRadius, northLine, eastLine);
				emitSphereVertex(sphereCenter, sphereRadius, southLine, eastLine);
				emitSphereVertex(sphereCenter, sphereRadius, southLine, westLine);

				emitSphereVertex(sphereCenter, sphereRadius, southLine, westLine);
				emitSphereVertex(sphereCenter, sphereRadius, southLine, eastLine);
				emitSphereVertex(sphereCenter, sphereRadius, northLine, eastLine);
				emitSphereVertex(sphereCenter, sphereRadius, northLine, westLine);
			}
		}
	}

	meshBuilder.End(false, true);

	if (!debug)
	{
		pRenderContext->OverrideAlphaWriteEnable(false, true);
		pRenderContext->OverrideColorWriteEnable(false, true);
	}
}

int CNEOGhostCapturePoint::DrawModel(int flags)
{
	int cl_neo_cap_zone_edge_value = cl_neo_cap_zone_edge.GetInt();

	if (!m_bIsActive || !m_pRingMaterial || !m_pRingJinraiMaterial || !m_pRingNsfMaterial || cl_neo_cap_zone_edge_value == NEO_CAP_EDGE_OFF) {
		return 0;
	}

	// Mirror the arrow color logic from CNEOHud_GhostCapPoint::DrawNeoHudElement exactly
	const int capTeam = owningTeamAlternate();
	Color ringColor = (capTeam == TEAM_ANY) ? COLOR_SPEC : ((capTeam == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF);

	auto *player = C_NEO_Player::GetLocalNEOPlayer();

	const Vector& capOrigin = GetAbsOrigin();
	int ringOpacity = 128;
	constexpr float MAX_VISIBILITY_RANGE = 768.0f;

	if (player)
	{
		const int playerTeam = player->GetTeamNumber();
		const bool playerIsPlaying = (playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF);

		if (playerIsPlaying && capTeam != TEAM_ANY && playerTeam != capTeam)
		{
			ringColor = COLOR_RED;
		}

		const Vector& playerOrigin = player->GetAbsOrigin();
		const float distanceToCap = playerOrigin.DistTo(capOrigin);

		if (distanceToCap > MAX_VISIBILITY_RANGE) {
			return 0; // Don't draw the ring if the player is too far away
		}

		// smooth fade out of ring opacity based on distance to player
		const float opacityCoef = distanceToCap / MAX_VISIBILITY_RANGE;
		ringOpacity = static_cast<int>(128 * (1.0f - opacityCoef));
	}

	ringColor[3] = ringOpacity;

	// Smoke writes no depth, so lay down invisible depth-only proxies for any
	// nearby smoke first; the ring below then fails its depth test where smoke
	// is in front of it and passes where it is in front of the smoke.
	if (cl_neo_cap_ring_smoke_proxy.GetBool())
	{
		CUtlVector<NeoCapSmokeProxy_t> smokeProxies;
		NeoCapRing_FindSmokeProxies(capOrigin, m_flCapzoneRadius, smokeProxies);
		if (!smokeProxies.IsEmpty())
		{
			NeoCapRing_DrawSmokeProxies(smokeProxies);
		}
	}

	// draw bars ring
	constexpr float SEGMENT_SIZE = 8.0f;
	constexpr float RING_BOTTOM = 4.0f;

	const int segments = (int)round(M_PI_F * 2.0f * m_flCapzoneRadius / SEGMENT_SIZE);
	const float zBottom = capOrigin.z + RING_BOTTOM;
	const float radius = m_flCapzoneRadius;

	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind(m_pRingMaterial);
	IMesh *pMesh = pRenderContext->GetDynamicMesh(true);
	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, segments);

	this->DrawBarRing(meshBuilder, segments, zBottom, SEGMENT_SIZE, radius, capOrigin, ringColor);

	meshBuilder.End(false, true);

	if (cl_neo_cap_zone_edge_value != NEO_CAP_EDGE_LINE_LOGO || capTeam == TEAM_ANY) {
		return 1;
	}

	// draw logo ring
	constexpr float LOGO_SIZE = 16.0f;
	constexpr float LOGO_BOTTOM = 2.0f;
	const float logoRadius = m_flCapzoneRadius - 1.0f;
	// draw logo only 5 times
	constexpr int LOGO_SEGMENTS = 5;
	const float zLogoBottom = capOrigin.z + LOGO_BOTTOM;

	pRenderContext->Bind(capTeam == TEAM_JINRAI ? m_pRingJinraiMaterial : m_pRingNsfMaterial);
	IMesh* pMeshTeam = pRenderContext->GetDynamicMesh(true);
	CMeshBuilder meshBuilderTeam;
	meshBuilderTeam.Begin(pMeshTeam, MATERIAL_QUADS, LOGO_SEGMENTS);

	this->DrawLogoRing(meshBuilderTeam, LOGO_SEGMENTS, zLogoBottom, LOGO_SIZE, logoRadius, capOrigin, ringColor);

	meshBuilderTeam.End(false, true);

	return 1;
}

void CNEOGhostCapturePoint::DrawBarRing(CMeshBuilder& builder, int segments, float zBottom, float segmentSize, float radius, Vector capOrigin, Color ringColor)
{
	// for portal effect add currentRotationInRad to angle0, maybe use it for the cap effect?
	//constexpr float ROTATION_SPEED = 45.0f;
	//const float currentRotationInRad = ROTATION_SPEED * gpGlobals->curtime * (M_PI_F / 180.0f);
	constexpr float CIRCLE_LENGTH = M_PI_F * 2.0f;

	for (int i = 0; i < segments; i++)
	{
		const float angle0 = ((float)i / segments) * CIRCLE_LENGTH;
		const float angle1 = ((float)(i + 1) / segments) * CIRCLE_LENGTH;
		this->DrawSegment(builder, angle0, angle1, zBottom, zBottom + segmentSize, radius, capOrigin, ringColor);
	}
}

void CNEOGhostCapturePoint::DrawLogoRing(CMeshBuilder& builder, int segmentsToFill, float zBottom, float segmentSize, float radius, Vector capOrigin, Color ringColor)
{
	constexpr float ROTATION_SPEED = 5.0f;
	constexpr float CIRCLE_LENGTH = M_PI_F * 2.0f;
	const float currentRotationInRad = ROTATION_SPEED * gpGlobals->curtime * (M_PI_F / 180.0f);
	// calculate the number of segments to fill based on the segment size and radius
	const int segmentsCountBySize = (int)round(CIRCLE_LENGTH * radius / segmentSize);

	for (int i = 0; i < segmentsToFill; i++)
	{
		const float angle0 = ((float)i / segmentsToFill) * CIRCLE_LENGTH + currentRotationInRad;
		// we take angle0 and add angle increment based on single segment size to get angle1
		// that way we are achieving a partial fill of the ring based on the segment size and radius
		const float angle1 = angle0 + ((float)1 / segmentsCountBySize) * CIRCLE_LENGTH;
		this->DrawSegment(builder, angle0, angle1, zBottom, zBottom + segmentSize, radius, capOrigin, ringColor);
	}
}

void CNEOGhostCapturePoint::DrawSegment(CMeshBuilder& builder, float angle0, float angle1, float zBottom, float zTop, float radius, Vector capOrigin, Color ringColor) {
	const float cos0 = cosf(angle0), sin0 = sinf(angle0);
	const float cos1 = cosf(angle1), sin1 = sinf(angle1);

	// Bottom at angle0
	builder.Position3f(capOrigin.x + cos0 * radius, capOrigin.y + sin0 * radius, zBottom);
	builder.Normal3f(cos0, sin0, 0.0f);
	builder.Color4ub(ringColor.r(), ringColor.g(), ringColor.b(), ringColor.a());
	builder.TexCoord2f(0, 0.0f, 1.0f);
	builder.AdvanceVertex();

	// Bottom at angle1
	builder.Position3f(capOrigin.x + cos1 * radius, capOrigin.y + sin1 * radius, zBottom);
	builder.Normal3f(cos1, sin1, 0.0f);
	builder.Color4ub(ringColor.r(), ringColor.g(), ringColor.b(), ringColor.a());
	builder.TexCoord2f(0, 1.0f, 1.0f);
	builder.AdvanceVertex();

	// Top at angle1
	builder.Position3f(capOrigin.x + cos1 * radius, capOrigin.y + sin1 * radius, zTop);
	builder.Normal3f(cos1, sin1, 0.0f);
	builder.Color4ub(ringColor.r(), ringColor.g(), ringColor.b(), ringColor.a());
	builder.TexCoord2f(0, 1.0f, 0.0f);
	builder.AdvanceVertex();

	// Top at angle0
	builder.Position3f(capOrigin.x + cos0 * radius, capOrigin.y + sin0 * radius, zTop);
	builder.Normal3f(cos0, sin0, 0.0f);
	builder.Color4ub(ringColor.r(), ringColor.g(), ringColor.b(), ringColor.a());
	builder.TexCoord2f(0, 0.0f, 0.0f);
	builder.AdvanceVertex();
}
#endif

void CNEOGhostCapturePoint::Precache(void)
{
	BaseClass::Precache();

	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

	#ifdef CLIENT_DLL
	PrecacheMaterial("effects/cap_zone_ring");
	PrecacheMaterial("effects/cap_zone_ring_nsf");
	PrecacheMaterial("effects/cap_zone_ring_jinrai");
	#endif
}

#ifdef GAME_DLL
void CNEOGhostCapturePoint::SetActive(bool isActive)
{
	m_bIsActive = isActive;
}

bool CNEOGhostCapturePoint::GetActive()
{
	return m_bIsActive;
}

void CNEOGhostCapturePoint::InputEnable(inputdata_t &inputData)
{
	if (m_iOwningTeam == TEAM_JINRAI || m_iOwningTeam == TEAM_NSF || m_iOwningTeam == TEAM_ANY)
	{
		SetActive(true);
	}
}

void CNEOGhostCapturePoint::InputDisable(inputdata_t &inputData)
{
	SetActive(false);
}
#endif
