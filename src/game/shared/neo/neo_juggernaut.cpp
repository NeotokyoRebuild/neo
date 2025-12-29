#include "neo_juggernaut.h"
#ifdef GAME_DLL
#include "engine/IEngineSound.h"
#include "explode.h"
#endif
#ifdef CLIENT_DLL
#include "model_types.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define USE_DURATION 5.0f
#define USE_DISTANCE_SQUARED 22500.0f

LINK_ENTITY_TO_CLASS(neo_juggernaut, CNEO_Juggernaut);

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST(CNEO_Juggernaut, DT_NEO_Juggernaut)
	SendPropBool(SENDINFO(m_bLocked)),
END_SEND_TABLE()
#else
#ifdef CNEO_Juggernaut
#undef CNEO_Juggernaut
#endif
IMPLEMENT_CLIENTCLASS_DT(C_NEO_Juggernaut, DT_NEO_Juggernaut, CNEO_Juggernaut)
	RecvPropBool(RECVINFO(m_bLocked)),
END_RECV_TABLE()
#define CNEO_Juggernaut C_NEO_Juggernaut
#endif

BEGIN_DATADESC(CNEO_Juggernaut)
#ifdef GAME_DLL
	DEFINE_USEFUNC(CNEO_Juggernaut::Use),
	DEFINE_KEYFIELD(m_bLocked, FIELD_BOOLEAN, "StartLocked"),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Lock", InputLock),
	DEFINE_INPUTFUNC(FIELD_VOID, "Unlock", InputUnlock),

	// Outputs
	DEFINE_OUTPUT(m_OnPlayerActivate, "OnPlayerActivate")
#endif
END_DATADESC()

#ifdef GAME_DLL
CNEO_Juggernaut::~CNEO_Juggernaut()
{
	if (m_hPush.Get())
	{
		m_hPush->Remove();
	}
}
#endif

void CNEO_Juggernaut::UpdateOnRemove()
{
	StopSound("HUD.CPCharge");
	BaseClass::UpdateOnRemove();
}

#ifdef GAME_DLL
void CNEO_Juggernaut::Precache(void)
{
	PrecacheModel("models/player/jgr.mdl");
	PrecacheModel("models/weapons/w_balc.mdl");
	PrecacheScriptSound("HUD.CPCharge");
	PrecacheScriptSound("HUD.CPCaptured");
	BaseClass::Precache();
}

void CNEO_Juggernaut::Spawn(void)
{
	Precache();

	SetModel("models/player/jgr.mdl");
	const int iSequence = LookupSequence("Boot_seq");
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		SetSequence(iSequence);
	}
	else
	{
		Warning("CNEO_Juggernaut missing animation!");
		SetSequence(0);
	}

	m_flWarpedPlaybackRate = SequenceDuration(iSequence) / USE_DURATION;

	if (m_bPostDeath)
	{
		Vector explOrigin = GetAbsOrigin() + NEO_JUGGERNAUT_VIEW_OFFSET;
		ExplosionCreate(explOrigin, GetAbsAngles(), this, 0, 128, SF_ENVEXPLOSION_NODAMAGE);
		SetPlaybackRate(-m_flWarpedPlaybackRate);
		SetCycle(1.0f);
		SetContextThink(&CNEO_Juggernaut::DisableSoftCollisionsThink, gpGlobals->curtime + TICK_INTERVAL, "SoftCollisionsThink");
	}
	else
	{
		SetPlaybackRate(0.0f);
	}

	SetMoveType(MOVETYPE_STEP);
	SetSolid(SOLID_BBOX);
	UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX); // Needs to be equal or smaller than the player's girth to avoid getting stuck
	SetFriction(100.0);
	SetSoftCollision(false);

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

	SetThink(&CNEO_Juggernaut::Think);
	SetNextThink(TICK_NEVER_THINK);
	SetContextThink(&CNEO_Juggernaut::AnimThink, gpGlobals->curtime + TICK_INTERVAL, "AnimThink");
	SetContextThink(&CNEO_Juggernaut::MakePushThink, gpGlobals->curtime + TICK_INTERVAL, "MakePushThink");

	BaseClass::Spawn();
}

void CNEO_Juggernaut::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_bLocked)
		return;

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
		SetPlaybackRate(m_flWarpedPlaybackRate);
		m_hPlayer->AddFlag(FL_FROZEN);
		UTIL_HudMessage(m_hPlayer, m_textParms, "BOOTING JGR56"); // TODO localise this text
		EmitSound("HUD.CPCharge");
	}
	else
	{
		m_bIsHolding = false;
	}
}

void CNEO_Juggernaut::Think(void)
{
	if (!m_bIsHolding || !m_hPlayer || !m_hPlayer->IsAlive() || !(m_hPlayer->m_nButtons & IN_USE))
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
		StopSound("HUD.CPCharge");
		EmitSound("HUD.CPCaptured");
		UTIL_HudMessage(m_hPlayer, m_textParms, ""); // Find a better way of hiding the text. This doesn't remove the old message from the user messages list and thus makes a weird overlapping visual bug

		m_hPlayer->RemoveFlag(FL_FROZEN);
		m_hPlayer->CreateRagdollEntity();
		m_hPlayer->Weapon_DropAllOnDeath(CTakeDamageInfo(this, this, 0, DMG_GENERIC));
		m_hPlayer->Teleport(&GetAbsOrigin(), &GetAbsAngles(), &vec3_origin);
		m_hPlayer->SnapEyeAngles(GetAbsAngles());

		m_hPlayer->BecomeJuggernaut();

		m_OnPlayerActivate.FireOutput(m_hPlayer, this);
		UTIL_Remove(this);

		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CNEO_Juggernaut::MakePushThink()
{
	if (IsMarkedForDeletion()) return;

	if (m_hPush.Get())
	{
		AssertMsg1(false, "push already exists; called %s multiply?", __FUNCTION__);
		return;
	}

	constexpr const char* pushEnt = "point_push";
	if (!(m_hPush = CreateEntityByName(pushEnt)))
	{
		Warning("%s: Failed to create %s\n", __FUNCTION__, pushEnt);
		return;
	}

	constexpr auto SF_PUSH_TEST_LOS = 0x0001, SF_PUSH_PLAYER = 0x0008;
	m_hPush->AddSpawnFlags(SF_PUSH_TEST_LOS | SF_PUSH_PLAYER);

	if (!m_hPush->KeyValue("magnitude", 200.f)) Assert(false);
	const float radius = BoundingRadius();
	Assert(radius <= PLAYER_USE_RADIUS);
	if (!m_hPush->KeyValue("radius", radius)) Assert(false);
	// inner_radius used for LOS calculations, so we don't push players through thin walls
	if (!m_hPush->KeyValue("inner_radius", 0.5f * radius)) Assert(false);

	m_hPush->Spawn();
	m_hPush->SetAbsOrigin(WorldSpaceCenter());
	m_hPush->SetParent(this);
	m_hPush->Activate();
	SetSoftCollision(m_bPostDeath);
}

void CNEO_Juggernaut::DisableSoftCollisionsThink()
{
	if (IsMarkedForDeletion()) return;

	[[maybe_unused]] CBaseEntity* ent;
	for (CEntitySphereQuery sphere(WorldSpaceCenter(), BoundingRadius(), FL_CLIENT | FL_FAKECLIENT | FL_NPC);
		(ent = sphere.GetCurrentEntity());
		sphere.NextEntity())
	{
		auto* combatChar = assert_cast<CBaseCombatCharacter*>(ent);
		if (!combatChar || !combatChar->IsAlive()) continue;
		// As long as there's a living player inside my collider, I can't go into "hard" player-blocking collisions mode
		// because they would get stuck. Try again later to see if they've moved.
		SetContextThink(&CNEO_Juggernaut::DisableSoftCollisionsThink, gpGlobals->curtime + 1.f, "SoftCollisionsThink");
		return;
	}

	// There's no stray players clipping inside the JGR currently, so we can go solid.
	SetSoftCollision(false);
}

void CNEO_Juggernaut::HoldCancel(void)
{
	if (m_hPlayer)
	{
		m_hPlayer->RemoveFlag(FL_FROZEN);
		UTIL_HudMessage(m_hPlayer, m_textParms, "");
	}
	SetNextThink(TICK_NEVER_THINK);
	SetPlaybackRate(-m_flWarpedPlaybackRate);
	m_hPlayer = nullptr;
	m_bIsHolding = false;
	StopSound("HUD.CPCharge");
}

void CNEO_Juggernaut::AnimThink(void) // Required for server controlled animation...
{
	StudioFrameAdvance();
	SetContextThink(&CNEO_Juggernaut::AnimThink, gpGlobals->curtime + TICK_INTERVAL, "AnimThink");
}

void CNEO_Juggernaut::SetSoftCollision(bool soft)
{
	// The collision mode to use when the JGR objective is solid and uncontrolled in the world.
	constexpr auto HARD_COLLISION = COLLISION_GROUP_VEHICLE;

	// The collision mode after the death explosion. Nearby players might be clipping inside the prop,
	// so we do a soft collision until it's confirmed no one's clipping anymore, so they don't get stuck.
	constexpr auto SOFT_COLLISION = COLLISION_GROUP_PLAYER;

	if (m_hPush.Get()) // Gotta have the soft collisions entity created and ready in order to do this
	{
		SetCollisionGroup(soft ? SOFT_COLLISION : HARD_COLLISION);
		m_hPush->AcceptInput(soft ? "Enable" : "Disable", this, this, variant_t{}, 0);
	}
	else
	{
		SetCollisionGroup(HARD_COLLISION);
	}
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
