#include "cbase.h"
#include "neo_model_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Array offsets
const int numTeams = 2;
const int numClasses = 3;

const char *playerModels[NEO_SKIN_ENUM_COUNT * numClasses * numTeams] {
	///////////////
	// NSF start //
	///////////////

	// Recons
	"models/player/nsf_gsf.mdl",
	"models/player/nsf_gsf2.mdl",
	"models/player/nsf_gsf3.mdl",

	// Assaults
	"models/player/nsf_gam.mdl",
	"models/player/nsf_gam2.mdl",
	"models/player/nsf_gam3.mdl",

	// Supports
	"models/player/nsf_ghm.mdl",
	"models/player/nsf_ghm2.mdl",
	"models/player/nsf_ghm3.mdl",

	//////////////////
	// Jinrai start //
	//////////////////

	// Recons
	"models/player/jinrai_msf.mdl",
	"models/player/jinrai_msf2.mdl",
	"models/player/jinrai_msf3.mdl",

	// Assaults
	"models/player/jinrai_mam.mdl",
	"models/player/jinrai_mam2.mdl",
	"models/player/jinrai_mam3.mdl",

	// Supports
	"models/player/jinrai_mhm.mdl",
	"models/player/jinrai_mhm2.mdl",
	"models/player/jinrai_mhm3.mdl"
};

const char *gibs[NEO_GIB_ENUM_COUNT * NEO_SKIN_ENUM_COUNT * numClasses * numTeams] {
	///////////////
	// NSF start //
	///////////////

	// Recons
	"models/player/gsf_dead_all.mdl",
	"models/player/gsf_dead_head.mdl",
	"models/player/gsf_dead_larm.mdl",
	"models/player/gsf_dead_lleg.mdl",
	"models/player/gsf_dead_rarm.mdl",
	"models/player/gsf_dead_rleg.mdl",

	"models/player/gsf2_dead_all.mdl",
	"models/player/gsf2_dead_head.mdl",
	"models/player/gsf2_dead_larm.mdl",
	"models/player/gsf2_dead_lleg.mdl",
	"models/player/gsf2_dead_rarm.mdl",
	"models/player/gsf2_dead_rleg.mdl",

	"models/player/gsf3_dead_all.mdl",
	"models/player/gsf3_dead_head.mdl",
	"models/player/gsf3_dead_larm.mdl",
	"models/player/gsf3_dead_lleg.mdl",
	"models/player/gsf3_dead_rarm.mdl",
	"models/player/gsf3_dead_rleg.mdl",

	// Assaults
	"models/player/gam_dead_all.mdl",
	"models/player/gam_dead_head.mdl",
	"models/player/gam_dead_larm.mdl",
	"models/player/gam_dead_lleg.mdl",
	"models/player/gam_dead_rarm.mdl",
	"models/player/gam_dead_rleg.mdl",

	"models/player/gam2_dead_all.mdl",
	"models/player/gam2_dead_head.mdl",
	"models/player/gam2_dead_larm.mdl",
	"models/player/gam2_dead_lleg.mdl",
	"models/player/gam2_dead_rarm.mdl",
	"models/player/gam2_dead_rleg.mdl",

	"models/player/gam3_dead_all.mdl",
	"models/player/gam3_dead_head.mdl",
	"models/player/gam3_dead_larm.mdl",
	"models/player/gam3_dead_lleg.mdl",
	"models/player/gam3_dead_rarm.mdl",
	"models/player/gam3_dead_rleg.mdl",

	// Supports
	"models/player/ghm_dead_all.mdl",
	"models/player/ghm_dead_head.mdl",
	"models/player/ghm_dead_larm.mdl",
	"models/player/ghm_dead_lleg.mdl",
	"models/player/ghm_dead_rarm.mdl",
	"models/player/ghm_dead_rleg.mdl",

	"models/player/ghm2_dead_all.mdl",
	"models/player/ghm2_dead_head.mdl",
	"models/player/ghm2_dead_larm.mdl",
	"models/player/ghm2_dead_lleg.mdl",
	"models/player/ghm2_dead_rarm.mdl",
	"models/player/ghm2_dead_rleg.mdl",

	"models/player/ghm3_dead_all.mdl",
	"models/player/ghm3_dead_head.mdl",
	"models/player/ghm3_dead_larm.mdl",
	"models/player/ghm3_dead_lleg.mdl",
	"models/player/ghm3_dead_rarm.mdl",
	"models/player/ghm3_dead_rleg.mdl",

	//////////////////
	// Jinrai start //
	//////////////////

	// Recons
	"models/player/msf_dead_all.mdl",
	"models/player/msf_dead_head.mdl",
	"models/player/msf_dead_larm.mdl",
	"models/player/msf_dead_lleg.mdl",
	"models/player/msf_dead_rarm.mdl",
	"models/player/msf_dead_rleg.mdl",

	"models/player/msf2_dead_all.mdl",
	"models/player/msf2_dead_head.mdl",
	"models/player/msf2_dead_larm.mdl",
	"models/player/msf2_dead_lleg.mdl",
	"models/player/msf2_dead_rarm.mdl",
	"models/player/msf2_dead_rleg.mdl",

	"models/player/msf3_dead_all.mdl",
	"models/player/msf3_dead_head.mdl",
	"models/player/msf3_dead_larm.mdl",
	"models/player/msf3_dead_lleg.mdl",
	"models/player/msf3_dead_rarm.mdl",
	"models/player/msf3_dead_rleg.mdl",

	// Assaults
	"models/player/mam_dead_all.mdl",
	"models/player/mam_dead_head.mdl",
	"models/player/mam_dead_larm.mdl",
	"models/player/mam_dead_lleg.mdl",
	"models/player/mam_dead_rarm.mdl",
	"models/player/mam_dead_rleg.mdl",

	"models/player/mam2_dead_all.mdl",
	"models/player/mam2_dead_head.mdl",
	"models/player/mam2_dead_larm.mdl",
	"models/player/mam2_dead_lleg.mdl",
	"models/player/mam2_dead_rarm.mdl",
	"models/player/mam2_dead_rleg.mdl",

	"models/player/mam3_dead_all.mdl",
	"models/player/mam3_dead_head.mdl",
	"models/player/mam3_dead_larm.mdl",
	"models/player/mam3_dead_lleg.mdl",
	"models/player/mam3_dead_rarm.mdl",
	"models/player/mam3_dead_rleg.mdl",

	// Supports
	"models/player/mhm_dead_all.mdl",
	"models/player/mhm_dead_head.mdl",
	"models/player/mhm_dead_larm.mdl",
	"models/player/mhm_dead_lleg.mdl",
	"models/player/mhm_dead_rarm.mdl",
	"models/player/mhm_dead_rleg.mdl",

	"models/player/mhm2_dead_all.mdl",
	"models/player/mhm2_dead_head.mdl",
	"models/player/mhm2_dead_larm.mdl",
	"models/player/mhm2_dead_lleg.mdl",
	"models/player/mhm2_dead_rarm.mdl",
	"models/player/mhm2_dead_rleg.mdl",

	"models/player/mhm3_dead_all.mdl",
	"models/player/mhm3_dead_head.mdl",
	"models/player/mhm3_dead_larm.mdl",
	"models/player/mhm3_dead_lleg.mdl",
	"models/player/mhm3_dead_rarm.mdl",
	"models/player/mhm3_dead_rleg.mdl"
};

// nsf recons have two gib limb varieties (male and female)
static const char* gibLimbs[(NEO_GIB_LIMB_ENUM_COUNT * numClasses * numTeams) + NEO_GIB_LIMB_ENUM_COUNT] {
	///////////////
	// NSF start //
	///////////////

	// Recon (male)
	"models/player/playergib/gsf3_head_gib.mdl",
	"models/player/playergib/gsf3_leftArm_gib.mdl",
	"models/player/playergib/gsf3_leftLeg_gib.mdl",
	"models/player/playergib/gsf3_rightArm_gib.mdl",
	"models/player/playergib/gsf3_rightLeg_gib.mdl",

	// Recon (female)
	"models/player/playergib/gsf_head_gib.mdl",
	"models/player/playergib/gsf_leftArm_gib.mdl",
	"models/player/playergib/gsf_leftLeg_gib.mdl",
	"models/player/playergib/gsf_rightArm_gib.mdl",
	"models/player/playergib/gsf_rightLeg_gib.mdl",

	// Assault
	"models/player/playergib/gam_head_gib.mdl",
	"models/player/playergib/gam_leftArm_gib.mdl",
	"models/player/playergib/gam_leftLeg_gib.mdl",
	"models/player/playergib/gam_rightArm_gib.mdl",
	"models/player/playergib/gam_rightLeg_gib.mdl",

	// Heavy
	"models/player/playergib/ghm_head_gib.mdl",
	"models/player/playergib/ghm_leftArm_gib.mdl",
	"models/player/playergib/ghm_leftLeg_gib.mdl",
	"models/player/playergib/ghm_rightArm_gib.mdl",
	"models/player/playergib/ghm_rightLeg_gib.mdl",

	//////////////////
	// Jinrai start //
	//////////////////

	// Recon
	"models/player/playergib/msf_head_gib.mdl",
	"models/player/playergib/msf_leftArm_gib.mdl",
	"models/player/playergib/msf_leftLeg_gib.mdl",
	"models/player/playergib/msf_rightArm_gib.mdl",
	"models/player/playergib/msf_rightLeg_gib.mdl",

	// Assault
	"models/player/playergib/mam_head_gib.mdl",
	"models/player/playergib/mam_leftArm_gib.mdl",
	"models/player/playergib/mam_leftLeg_gib.mdl",
	"models/player/playergib/mam_rightArm_gib.mdl",
	"models/player/playergib/mam_rightLeg_gib.mdl",

	// Support
	"models/player/playergib/mhm_head_gib.mdl",
	"models/player/playergib/mhm_leftArm_gib.mdl",
	"models/player/playergib/mhm_leftLeg_gib.mdl",
	"models/player/playergib/mhm_rightArm_gib.mdl",
	"models/player/playergib/mhm_rightLeg_gib.mdl",
};

const char *viewModels[NEO_VM_ENUM_COUNT * numTeams] {
	///////////////
	// NSF start //
	///////////////

	"models/weapons/v_nsf_aa13.mdl",
	"models/weapons/v_nsf_detpack.mdl",
	"models/weapons/v_nsf_frag.mdl",
	"models/weapons/v_nsf_jitte.mdl",
	"models/weapons/v_nsf_jittes.mdl",
	"models/weapons/v_nsf_klya9.mdl",
	"models/weapons/v_nsf_knife.mdl",
	"models/weapons/v_nsf_milso.mdl",
	"models/weapons/v_nsf_milso_silenced.mdl",
	"models/weapons/v_nsf_mosok41.mdl",
	"models/weapons/v_nsf_mosok41s.mdl",
	"models/weapons/v_nsf_mosok41l.mdl",
	"models/weapons/v_nsf_mpn.mdl",
	"models/weapons/v_nsf_mpn_s.mdl",
	"models/weapons/v_nsf_mx.mdl",
	"models/weapons/v_nsf_mxs.mdl",
	"models/weapons/v_nsf_proxmine.mdl",
	"models/weapons/v_nsf_pz.mdl",
	"models/weapons/v_nsf_shotgun.mdl",
	"models/weapons/v_nsf_smokenade.mdl",
	"models/weapons/v_nsf_srm.mdl",
	"models/weapons/v_nsf_srm_s.mdl",
	"models/weapons/v_nsf_srs.mdl",
	"models/weapons/v_nsf_tachi.mdl",
	"models/weapons/v_nsf_zr68c.mdl",
	"models/weapons/v_nsf_zr68l.mdl",
	"models/weapons/v_nsf_zr68s.mdl",
	"models/gameplay/v_nsf_ghost.mdl",
#ifdef INCLUDE_WEP_PBK
	"models/weapons/v_nsf_pbk56.mdl",
#endif

	//////////////////
	// Jinrai start //
	//////////////////

	"models/weapons/v_jinrai_aa13.mdl",
	"models/weapons/v_jinrai_detpack.mdl",
	"models/weapons/v_jinrai_frag.mdl",
	"models/weapons/v_jinrai_jitte.mdl",
	"models/weapons/v_jinrai_jittes.mdl",
	"models/weapons/v_jinrai_klya9.mdl",
	"models/weapons/v_jinrai_knife.mdl",
	"models/weapons/v_jinrai_milso.mdl",
	"models/weapons/v_jinrai_milso_silenced.mdl",
	"models/weapons/v_jinrai_mosok41.mdl",
	"models/weapons/v_jinrai_mosok41s.mdl",
	"models/weapons/v_jinrai_mosok41l.mdl",
	"models/weapons/v_jinrai_mpn.mdl",
	"models/weapons/v_jinrai_mpn_s.mdl",
	"models/weapons/v_jinrai_mx.mdl",
	"models/weapons/v_jinrai_mxs.mdl",
	"models/weapons/v_jinrai_proxmine.mdl",
	"models/weapons/v_jinrai_pz.mdl",
	"models/weapons/v_jinrai_shotgun.mdl",
	"models/weapons/v_jinrai_smokenade.mdl",
	"models/weapons/v_jinrai_srm.mdl",
	"models/weapons/v_jinrai_srm_s.mdl",
	"models/weapons/v_jinrai_srs.mdl",
	"models/weapons/v_jinrai_tachi.mdl",
	"models/weapons/v_jinrai_zr68c.mdl",
	"models/weapons/v_jinrai_zr68l.mdl",
	"models/weapons/v_jinrai_zr68s.mdl",
	"models/gameplay/v_jinrai_ghost.mdl",
#ifdef INCLUDE_WEP_PBK
	"models/weapons/v_jinrai_pbk56.mdl",
#endif
};

const char *weapons[NEO_WEP_MDL_ENUM_COUNT] {
	"models/weapons/w_aa13.mdl",
	"models/weapons/w_detpack.mdl",
	"models/weapons/w_detremote.mdl",
	"models/weapons/w_eq_fraggrenade.mdl",
	"models/weapons/w_eq_fraggrenade_thrown.mdl",
	"models/weapons/w_frag.mdl",
	"models/weapons/w_frag_thrown.mdl",
	"models/weapons/w_jitte.mdl",
	"models/weapons/w_jittes.mdl",
	"models/weapons/w_klya9.mdl",
	"models/weapons/w_knife.mdl",
	"models/weapons/w_milso.mdl",
	"models/weapons/w_milso_silenced.mdl",
	"models/weapons/w_mosok41.mdl",
	"models/weapons/w_mosok41s.mdl",
	"models/weapons/w_mosok41l.mdl",
	"models/weapons/w_mpn.mdl",
	"models/weapons/w_mpn_folded.mdl",
	"models/weapons/w_mpn_silenced.mdl",
	"models/weapons/w_mpn_silenced_folded.mdl",
	"models/weapons/w_mx.mdl",
	"models/weapons/w_mxs.mdl",
	"models/weapons/w_proxmine.mdl",
	"models/weapons/w_pz252.mdl",
	"models/weapons/w_smac.mdl",
	"models/weapons/w_smokenade.mdl",
	"models/weapons/w_smokenade_thrown.mdl",
	"models/weapons/w_srm.mdl",
	"models/weapons/w_srm_s.mdl",
	"models/weapons/w_srs.mdl",
	"models/weapons/w_supa7.mdl",
	"models/weapons/w_tachi.mdl",
	"models/weapons/w_zr68c.mdl",
	"models/weapons/w_zr68l.mdl",
	"models/weapons/w_zr68s.mdl",
	"models/gameplay/w_ghost.mdl",
#ifdef INCLUDE_WEP_PBK
	"models/weapons/w_pbk56s.mdl",
#endif
};

const char *vipModel = "models/player/vip.mdl";
const char *vipModelDead = "models/player/vip_dead.mdl";
const char *vipSmacViewModel = "models/weapons/v_vip_smac.mdl";

const char *jgrModel = "models/player/jgr.mdl";

static inline void PrecachePlayerModels( void )
{
	const int size = ARRAYSIZE(playerModels);
	for (int i = 0; i < size; i++)
	{
		CBaseEntity::PrecacheModel(playerModels[i]);
	}

	CBaseEntity::PrecacheModel(vipModel);
	CBaseEntity::PrecacheModel(jgrModel);
}

static inline void PrecacheGibs( void )
{
	const int size = ARRAYSIZE(gibs);
	for (int i = 0; i < size; i++)
	{
		CBaseEntity::PrecacheModel(gibs[i]);
	}

	CBaseEntity::PrecacheModel(vipModelDead);
}

static inline void PrecacheGibLimbs(void)
{
	const int size = ARRAYSIZE(gibLimbs);
	for (int i = 0; i < size; i++)
	{
		CBaseEntity::PrecacheModel(gibLimbs[i]);
	}
}

static inline void PrecacheViewModels( void )
{
	const int size = ARRAYSIZE(viewModels);
	for (int i = 0; i < size; i++)
	{
		CBaseEntity::PrecacheModel(viewModels[i]);
	}

	CBaseEntity::PrecacheModel(vipSmacViewModel);
}

static inline void PrecacheWeapons( void )
{
	const int size = ARRAYSIZE(weapons);
	for (int i = 0; i < size; i++)
	{
		CBaseEntity::PrecacheModel(weapons[i]);
	}
}

// Precaches all Neotokyo player and weapon related models.
void CNEOModelManager::Precache( void ) const
{
	DevMsg("CNEOModelManager::Precache\n");

	PrecachePlayerModels();
	PrecacheViewModels();
	PrecacheWeapons();
	PrecacheGibs();
	PrecacheGibLimbs();

	CBaseEntity::PrecacheModel("models/player/jinrai_mamanims.mdl");
	CBaseEntity::PrecacheModel("models/player/jinrai_mhmanims.mdl");
	CBaseEntity::PrecacheModel("models/player/jinrai_msfanims.mdl");
	CBaseEntity::PrecacheModel("models/player/jinrai_msfanims2.mdl");
	CBaseEntity::PrecacheModel("models/player/nsf_gamanims.mdl");
	CBaseEntity::PrecacheModel("models/player/nsf_ghmanims.mdl");
	CBaseEntity::PrecacheModel("models/player/nsf_gsfanims.mdl");
	CBaseEntity::PrecacheModel("models/player/nsf_gsfanims2.mdl");
	//CBaseEntity::PrecacheModel("models/player/nsf_nsfanims2.mdl");
	CBaseEntity::PrecacheModel("models/player/vip_anims.mdl");

	CBaseEntity::PrecacheModel("particle/fire.vmt");

	PrecacheMaterial("toc.vmt");
	PrecacheMaterial("toc2.vmt");
	PrecacheMaterial("models/player/toc.vmt");
	PrecacheMaterial("toc_remake_pass1.vmt");
	PrecacheMaterial("toc_remake_pass2.vmt");
	PrecacheMaterial("toc_remake_vm.vmt");

	//PrecacheMaterial("water/ntwater_ivy");

	PrecacheMaterial("dev/motion_third.vmt");
	PrecacheMaterial("dev/thermal_model.vmt");
	PrecacheMaterial("dev/thermal_ragdoll_model.vmt");
	PrecacheMaterial("dev/thermal_base_animating_model.vmt");
	PrecacheMaterial("dev/thermal_view_model.vmt");
	PrecacheMaterial("dev/thermal_weapon_model.vmt");
	PrecacheMaterial("dev/thermal_grenade_projectile_model.vmt");
}

static inline int GetTeamArrOffset(int iTeam)
{
	return iTeam == TEAM_NSF ? 0 : 1;
}

// Returns a third person corpse, or if defined, a related gib body part.
const char* CNEOModelManager::GetCorpseModel(NeoSkin nSkin, NeoClass nClass,
	int iTeam, NeoGib nGib) const
{
	if (nClass == NEO_CLASS_VIP)
	{
		return vipModelDead;
	}

	const int index =
		(NEO_GIB_ENUM_COUNT * NEO_SKIN_ENUM_COUNT * numClasses * GetTeamArrOffset(iTeam))
		+ (NEO_GIB_ENUM_COUNT * NEO_SKIN_ENUM_COUNT * nClass)
		+ (NEO_GIB_ENUM_COUNT * nSkin)
		+ nGib;

	if (index < 0 || index >= ARRAYSIZE(gibs))
	{
		Assert(false);
		return gibs[0];
	}

	return gibs[index];
}

// Returns a gib body part.
const char* CNEOModelManager::GetGibModel(NeoSkin nSkin, NeoClass nClass,
	int iTeam, NeoGibLimb nGib) const
{
	// NEO FIXME (Rain): Shouldn't we get the "models/nt/vipgib..." variants here?
	if (nClass == NEO_CLASS_VIP)
	{
		return "models/player/playergib/gsf3_head_gib.mdl";
	}
	
	const auto isMaleNsfRecon = (nSkin == NEO_SKIN_THIRD) &&
		(iTeam == TEAM_NSF) &&
		(nClass == NEO_CLASS_RECON);

	const int index =
		(NEO_GIB_LIMB_ENUM_COUNT * numClasses * GetTeamArrOffset(iTeam))
		+ (NEO_GIB_LIMB_ENUM_COUNT * nClass)
		+ nGib
		// Because the NSF recon has gender specific gibs,
		// offset all gibs except the male NSF recon ones.
		+ (isMaleNsfRecon ? 0 : NEO_GIB_LIMB_ENUM_COUNT);

	if (index < 0 || index >= ARRAYSIZE(gibLimbs))
	{
		Assert(false);
		return gibLimbs[0];
	}

	return gibLimbs[index];
}

// Returns a third person player model.
// NEO FIXME (Rain): this is sometimes off. Are we indexing incorrectly, or is the cvar logic flawed?
const char *CNEOModelManager::GetPlayerModel(NeoSkin nSkin,
	NeoClass nClass, int iTeam) const
{
	if (nClass == NEO_CLASS_VIP)
	{
		return vipModel;
	}

	if (nClass == NEO_CLASS_JUGGERNAUT)
	{
		return jgrModel;
	}

	// Unspecified skin number, give a skin randomly.
	if ((int)nSkin == -1)
	{
		nSkin = (NeoSkin)RandomInt(NEO_SKIN_FIRST, NEO_SKIN_THIRD);
	}

	// We don't know what class this player wants (they probably just joined),
	// give them an assault model as placeholder.
	if ((int)nClass == -1)
	{
		nClass = NEO_CLASS_ASSAULT;
	}

	const int index =
		(NEO_SKIN_ENUM_COUNT * numClasses * GetTeamArrOffset(iTeam))
		+ (NEO_SKIN_ENUM_COUNT * nClass)
		+ nSkin;

	if (index < 0 || index >= ARRAYSIZE(playerModels))
	{
		Assert(false);
		return playerModels[0];
	}

	return playerModels[index];
}

// Returns a first person view model.
const char *CNEOModelManager::GetViewModel(NeoViewmodel nWepVm, int iTeam) const
{
	if (nWepVm == NEO_VM_VIP_SMAC)
	{
		return vipSmacViewModel;
	}

	const int index = (NEO_VM_ENUM_COUNT * GetTeamArrOffset(iTeam)) + nWepVm;

	if (index < 0 || index >= ARRAYSIZE(viewModels))
	{
		Assert(false);
		return viewModels[0];
	}

	return viewModels[index];
}

// Returns a third person weapon model.
const char *CNEOModelManager::GetWeaponModel(NeoWeaponModel nWep) const
{
	if (nWep < 0 || nWep >= ARRAYSIZE(weapons))
	{
		Assert(false);
		return weapons[0];
	}

	return weapons[nWep];
}
