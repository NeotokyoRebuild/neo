#include "cbase.h"
#include "weapon_neobase_ar.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(NEOAssaultRifle, DT_NEOAssaultRifle);

BEGIN_NETWORK_TABLE(CNEOAssaultRifle, DT_NEOAssaultRifle)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CNEOAssaultRifle)
END_PREDICTION_DATA()

BEGIN_DATADESC(CNEOAssaultRifle)
	DEFINE_FIELD(m_nShotsFired, FIELD_INTEGER),
	DEFINE_FIELD(m_flNextSoundTime, FIELD_TIME),
END_DATADESC()

CNEOAssaultRifle::CNEOAssaultRifle()
{
}

const Vector &CNEOAssaultRifle::GetBulletSpread()
{
	static Vector cone = VECTOR_CONE_3DEGREES;
	return cone;
}

void CNEOAssaultRifle::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	// Abort here to handle burst and auto fire modes
	if ((UsesClipsForAmmo1() && m_iClip1 == 0) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType)))
		return;

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	while (m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		// MUST call sound before removing a round from the clip of a CHLMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if (UsesClipsForAmmo1())
	{
		if (iBulletsToFire > m_iClip1)
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	CHL2MP_Player *pHL2MPPlayer = ToHL2MPPlayer(pPlayer);

	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pHL2MPPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	info.m_vecSpread = pHL2MPPlayer->GetAttackSpread(this);
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;
	FireBullets(info);

	//Factor in the view kick
	AddViewKick();

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	SendWeaponAnim(GetPrimaryAttackActivity());
	pPlayer->SetAnimation(PLAYER_ATTACK1);
}

void CNEOAssaultRifle::FireBullets(const FireBulletsInfo_t &info)
{
	if (CBasePlayer *pPlayer = ToBasePlayer(GetOwner()))
	{
		pPlayer->FireBullets(info);
	}
}

void CNEOAssaultRifle::DoAssaultRifleGunKick(CBasePlayer *pPlayer, float dampEasy,
	float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime)
{
#define	KICK_MIN_X			0.2f	//Degrees
#define	KICK_MIN_Y			0.2f	//Degrees
#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;

	//Find how far into our accuracy degradation we are
	float duration = (fireDurationTime > slideLimitTime) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset(10);

	//Apply this to the view angles as well
	vecScratch.x = -(KICK_MIN_X + (maxVerticleKickAngle * kickPerc));
	vecScratch.y = -(KICK_MIN_Y + (maxVerticleKickAngle * kickPerc)) / 3;
	vecScratch.z = KICK_MIN_Z + (maxVerticleKickAngle * kickPerc) / 8;

	RandomSeed(iSeed);

	//Wibble left and right
	if (RandomInt(-1, 1) >= 0)
		vecScratch.y *= -1;

	iSeed++;

	//Wobble up and down
	if (RandomInt(-1, 1) >= 0)
		vecScratch.z *= -1;

	//Clip this to our desired min/max
	UTIL_ClipPunchAngleOffset(vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle(24.0f, 3.0f, 1.0f));

	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch(vecScratch * 0.5);
}

//-----------------------------------------------------------------------------
// Purpose: Reset our shots fired
//-----------------------------------------------------------------------------
bool CNEOAssaultRifle::Deploy(void)
{
	m_nShotsFired = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Make enough sound events to fill the estimated think interval
// returns: number of shots needed
//-----------------------------------------------------------------------------
int CNEOAssaultRifle::WeaponSoundRealtime(WeaponSound_t shoot_type)
{
	int numBullets = 0;

	// ran out of time, clamp to current
	if (m_flNextSoundTime < gpGlobals->curtime)
	{
		m_flNextSoundTime = gpGlobals->curtime;
	}

	// make enough sound events to fill up the next estimated think interval
	float dt = clamp(m_flAnimTime - m_flPrevAnimTime, 0, 0.2);
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound(SINGLE_NPC, m_flNextSoundTime);
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound(SINGLE_NPC, m_flNextSoundTime);
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}

	return numBullets;
}

void CNEOAssaultRifle::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Debounce the recoiling counter
	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_nShotsFired = 0;
	}

	BaseClass::ItemPostFrame();
}