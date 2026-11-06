#include "cbase.h"
#include "weapon_srs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSRS, DT_WeaponSRS)

BEGIN_NETWORK_TABLE(CWeaponSRS, DT_WeaponSRS)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bNeedsBolting)),
	RecvPropTime(RECVINFO(m_flBoltHomeTime)),
#else
	SendPropBool(SENDINFO(m_bNeedsBolting)),
	SendPropTime(SENDINFO(m_flBoltHomeTime)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSRS)
	DEFINE_NEO_BASE_WEP_PREDICTION
	DEFINE_PRED_FIELD(m_bNeedsBolting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flBoltHomeTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponSRS)

LINK_ENTITY_TO_CLASS(weapon_srs, CWeaponSRS);

PRECACHE_WEAPON_REGISTER(weapon_srs);

// The bolt cycle after a shot; its phase table in the weapon script holds the moment the bolt is home
static constexpr const char *VM_SEQ_CHARGE = "charge";

CWeaponSRS::CWeaponSRS()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
	m_bNeedsBolting = false;
	m_flBoltHomeTime = 0.0f;

	m_weaponSeeds = {
		"srspx",
		"srspy",
		"srsrx",
		"srsry",
	};
}

void CWeaponSRS::PrimaryAttack()
{
	if (ShootingIsPrevented() || !m_iClip1 || !m_bTriggerReset)
	{
		Assert(!(m_iClip1 < 0));
		return BaseClass::PrimaryAttack();
	}

	const bool tryingToShootTooFast =
		(gpGlobals->curtime < m_flLastAttackTime + GetFireRate());
	if (tryingToShootTooFast)
	{
		return;
	}

	m_bNeedsBolting = true;
	BaseClass::PrimaryAttack();
}

void CWeaponSRS::ItemPreFrame()
{
	if (!m_bNeedsBolting)
	{
		return BaseClass::ItemPreFrame();
	}

	if (m_flLastAttackTime + 0.08f <= gpGlobals->curtime)
	{ // Primary attack animation finished, begin chambering a round
		if (auto* pOwner = ToNEOPlayer(GetOwner())) {
			if (pOwner->m_nButtons & IN_ATTACK)
			{
				return BaseClass::ItemPreFrame();
			}
		}
		// Only queue the bolting anim once; we want to allow the player
		// to cancel this with manual weapon quick-switching.
		SendWeaponAnim(ACT_VM_PULLBACK);
		WeaponSound(SPECIAL1);
		m_bNeedsBolting = false;

		if (IsReloadPhaseSystemEnabled())
		{
			// the script's "charge" table gives when the bolt is home with the next round
			const CNEOWeaponInfo &info = GetNEOWpnData();
			const int iTable = info.FindReloadTable(VM_SEQ_CHARGE);
			const bool bHasCredit = iTable >= 0 && info.m_reloadTables[iTable].iNumRows > 0;
			m_flBoltHomeTime = gpGlobals->curtime + (bHasCredit ? info.m_reloadTables[iTable].rows[0].flCredit : 0.0f);
		}
	}

	BaseClass::ItemPreFrame();
}

bool CWeaponSRS::Deploy()
{
	// This is on purpose; the player is allowed to cancel the bolting animation
	// with a quick-switch input.
	m_bNeedsBolting = false;
	m_flBoltHomeTime = 0.0f;

	return BaseClass::Deploy();
}

bool CWeaponSRS::Reload()
{
	// The reloading animation already contains the bolting motion;
	// we don't want to bolt twice.
	m_bNeedsBolting = false;
	m_flBoltHomeTime = 0.0f;

	return BaseClass::Reload();
}

bool CWeaponSRS::IsRoundChambered() const
{
	if (!IsReloadPhaseSystemEnabled())
	{
		return BaseClass::IsRoundChambered();
	}

	if (m_bNeedsBolting || gpGlobals->curtime < m_flBoltHomeTime)
	{
		return false;
	}
	return BaseClass::IsRoundChambered();
}

bool CWeaponSRS::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}
