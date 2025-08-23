#include "neo_juggernaut.h"
#include "engine/IEngineSound.h"
#ifdef GAME_DLL
#include "explode.h"
#endif
#ifdef CLIENT_DLL
#include "model_types.h"
#endif

#define USE_DURATION 4.0f
#define USE_DISTANCE_SQUARED 22500.0f

LINK_ENTITY_TO_CLASS(neo_juggernaut, CNEO_Juggernaut);

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST(CNEO_Juggernaut, DT_NEO_Juggernaut)
	SendPropFloat(SENDINFO(m_flHoldStartTime)),
	SendPropBool(SENDINFO(m_bIsHolding)),
END_SEND_TABLE()
#else
IMPLEMENT_CLIENTCLASS_DT(CNEO_Juggernaut, DT_NEO_Juggernaut, CNEO_Juggernaut)
	RecvPropFloat(RECVINFO(m_flHoldStartTime)),
	RecvPropBool(RECVINFO(m_bIsHolding)),
END_RECV_TABLE()
#endif

BEGIN_DATADESC(CNEO_Juggernaut)
#ifdef GAME_DLL
	DEFINE_USEFUNC(CNEO_Juggernaut::Use),

	// Outputs
	DEFINE_OUTPUT(m_OnPlayerActivate, "OnPlayerActivate")
#endif
END_DATADESC()

void CNEO_Juggernaut::Precache(void)
{
	PrecacheModel("models/player/jgr.mdl");
	PrecacheModel("models/weapons/w_balc.mdl");
	BaseClass::Precache();
}

void CNEO_Juggernaut::Spawn(void)
{
	Precache();

	m_flHoldStartTime = 0.0f;
	m_bIsHolding = false;

	SetModel("models/player/jgr.mdl");
	SetMoveType(MOVETYPE_STEP);
	SetSolid(SOLID_BBOX);
#ifdef GAME_DLL
	UTIL_SetSize(this, -Vector(15, 15, 0), Vector(15, 15, 90)); // Needs to be equal or smaller than the player's girth to avoid getting stuck

	m_textParms.channel = 0;
	m_textParms.x = 0.42;
	m_textParms.y = 0.4;
	m_textParms.effect = 0;
	m_textParms.fadeinTime = 0;
	m_textParms.fadeoutTime = 0;
	m_textParms.holdTime = USE_DURATION;
	m_textParms.fxTime = 0;

	m_textParms.r1 = 200;
	m_textParms.g1 = 200;
	m_textParms.b1 = 200;
	m_textParms.a1 = 100;
	m_textParms.r2 = 200;
	m_textParms.g2 = 200;
	m_textParms.b2 = 200;
	m_textParms.a2 = 100;
#endif
	SetCollisionGroup(COLLISION_GROUP_PLAYER);
	SetFriction(100.0);
	
#ifdef GAME_DLL
	CBaseEntity *pWeaponModel = CreateEntityByName("prop_dynamic");
	if (pWeaponModel)
	{
		pWeaponModel->SetModel("models/weapons/w_balc.mdl");
		pWeaponModel->SetParent(this);
		pWeaponModel->AddEffects(EF_BONEMERGE);
	}
	else
	{
		Warning("Failed to create weapon model for CNEO_Juggernaut!");
	}
#endif

	SetThink(&CNEO_Juggernaut::Think);
	SetNextThink(TICK_NEVER_THINK);

	BaseClass::Spawn();
}

void CNEO_Juggernaut::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = ToBasePlayer(pActivator);
	if (!pPlayer || !pPlayer->IsAlive())
		return;

	CNEO_Player* pNEOPlayer = static_cast<CNEO_Player*>(pPlayer);
	if (pNEOPlayer->GetClass() == NEO_CLASS_JUGGERNAUT)
		return;

	if (m_bIsHolding && m_hPlayer.Get() != pNEOPlayer) // One player at a time
		return;

	// Either I'm dumb, or USE_TYPE is non-functional. Always sends USE_TOGGLE no matter what. Just checking the key instead
	if (pNEOPlayer->m_afButtonPressed & IN_USE)
	{
		m_bIsHolding = true;
		m_hPlayer = pNEOPlayer;
		m_flHoldStartTime = gpGlobals->curtime;
		SetNextThink(gpGlobals->curtime + 0.1f);
		m_hPlayer->AddFlag(FL_FROZEN);
#ifdef GAME_DLL
		UTIL_HudMessage(m_hPlayer, m_textParms, "BOOTING JGR56"); // TODO localise this text
#endif
	}
	else
	{
		m_bIsHolding = false;
	}
}

void CNEO_Juggernaut::Think(void)
{
	if (!m_bIsHolding || !m_hPlayer || !m_hPlayer->IsAlive())
	{
		HoldCancel();
		return;
	}

	if (((m_hPlayer->GetAbsOrigin() - GetAbsOrigin()).LengthSqr()) > USE_DISTANCE_SQUARED)
	{
		HoldCancel();
		return;
	}

	float flTimeHeld = gpGlobals->curtime - m_flHoldStartTime;
	if (flTimeHeld >= USE_DURATION)
	{
		m_bIsHolding = false;
		SetNextThink(TICK_NEVER_THINK);
#ifdef GAME_DLL
		m_hPlayer->CreateRagdollEntity();
		m_hPlayer->Weapon_DropAll(false);
#endif
		m_hPlayer->SetAbsVelocity(vec3_origin);
		m_hPlayer->SetAbsOrigin(GetAbsOrigin());
		m_hPlayer->SetAbsAngles(GetAbsAngles());
		m_hPlayer->RemoveFlag(FL_FROZEN);
#ifdef GAME_DLL
		m_hPlayer->SnapEyeAngles(GetAbsAngles());
		UTIL_HudMessage(m_hPlayer, m_textParms, ""); // Find a better way of hiding the text. This doesn't remove the old message from the user messages list and thus makes a weird overlapping visual bug

		m_hPlayer->BecomeJuggernaut();

		m_OnPlayerActivate.FireOutput(m_hPlayer, this);
		UTIL_Remove(this);
#endif
		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CNEO_Juggernaut::HoldCancel(void)
{
	if (m_hPlayer)
	{
		m_hPlayer->RemoveFlag(FL_FROZEN);
#ifdef GAME_DLL
		UTIL_HudMessage(m_hPlayer, m_textParms, "");
#endif
	}
	SetNextThink(TICK_NEVER_THINK);
	m_hPlayer = nullptr;
	m_bIsHolding = false;
}

#ifdef GAME_DLL
void CNEO_Juggernaut::PostDeathEffects(void)
{
	Vector explOrigin = GetAbsOrigin() + NEO_JUGGERNAUT_VIEW_OFFSET;
	ExplosionCreate(explOrigin, GetAbsAngles(), this, 0, 128, SF_ENVEXPLOSION_NODAMAGE);
}
#endif

#ifdef CLIENT_DLL
int CNEO_Juggernaut::DrawModel(int flags)
{
	if (flags & STUDIO_IGNORE_NEO_EFFECTS || !(flags & STUDIO_RENDER))
	{
		return BaseClass::DrawModel(flags);
	}

	auto pTargetPlayer = C_NEO_Player::GetVisionTargetNEOPlayer();
	if (!pTargetPlayer)
	{
		Assert(false);
		return BaseClass::DrawModel(flags);
	}

	bool inThermalVision = pTargetPlayer ? (pTargetPlayer->IsInVision() && pTargetPlayer->GetClass() == NEO_CLASS_SUPPORT) : false;

	int ret = 0;
	if (inThermalVision)
	{
		IMaterial* pass = materials->FindMaterial("dev/thermal_model", TEXTURE_GROUP_MODEL);
		modelrender->ForcedMaterialOverride(pass);
		ret = BaseClass::DrawModel(flags);
		modelrender->ForcedMaterialOverride(nullptr);
	}
	else
	{
		ret = BaseClass::DrawModel(flags);
	}

	return ret;
}
#endif