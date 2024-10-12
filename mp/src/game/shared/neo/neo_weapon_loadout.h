#ifndef NEO_WEAPON_LOADOUT_H
#define NEO_WEAPON_LOADOUT_H
#ifdef _WIN32
#pragma once
#endif

#define DEV_WEAPON_LOADOUTS 12
#define RECON_WEAPON_LOADOUTS 9
#define ASSAULT_WEAPON_LOADOUTS 12
#define SUPPORT_WEAPON_LOADOUTS 9
#define VIP_WEAPON_LOADOUTS 9

class CLoadoutWeaponClass
{
public:
	const char* m_szWeaponName;
	int m_iWeaponPrice;
	const char* m_szVguiImage;
	const char* m_szVguiImageNo; // I don't know how to concatenate char * to const char * please don't bully
	const char* m_szWeaponEntityName;
	const char* m_szAmmoType;
	
	CLoadoutWeaponClass(const char* weaponName, int weaponPrice, const char* vguiImage, const char* vguiImageNo, const char* weaponEntityName, const char* ammoType)
	{
		m_szWeaponName = weaponName;
		m_iWeaponPrice = weaponPrice;
		m_szVguiImage = vguiImage;
		m_szVguiImageNo = vguiImageNo;
		m_szWeaponEntityName = weaponEntityName;
		m_szAmmoType = ammoType;
	}
};

class CNEOWeaponLoadout
{
public:
	static CLoadoutWeaponClass s_DevLoadoutWeapons[DEV_WEAPON_LOADOUTS];
	static CLoadoutWeaponClass s_ReconLoadoutWeapons[RECON_WEAPON_LOADOUTS];
	static CLoadoutWeaponClass s_AssaultLoadoutWeapons[ASSAULT_WEAPON_LOADOUTS];
	static CLoadoutWeaponClass s_SupportLoadoutWeapons[SUPPORT_WEAPON_LOADOUTS];
	static CLoadoutWeaponClass s_VIPLoadoutWeapons[VIP_WEAPON_LOADOUTS];

	static int GetNumberOfLoadoutWeapons(int rank, int classType, int isDev)
	{
		if (classType < NEO_CLASS_RECON || classType > NEO_CLASS_VIP)
		{ // We don't have a loadout for this class
			Assert(false);
			return 0;
		}

		if (isDev)
		{
			return iterateThroughLoadout(rank, s_DevLoadoutWeapons, DEV_WEAPON_LOADOUTS);
		}
		switch (classType) {
		case NEO_CLASS_RECON:
			return iterateThroughLoadout(rank, s_ReconLoadoutWeapons, RECON_WEAPON_LOADOUTS);
		case NEO_CLASS_ASSAULT:
			return iterateThroughLoadout(rank, s_AssaultLoadoutWeapons, ASSAULT_WEAPON_LOADOUTS);
		case NEO_CLASS_SUPPORT:
			return iterateThroughLoadout(rank, s_SupportLoadoutWeapons, SUPPORT_WEAPON_LOADOUTS);
		case NEO_CLASS_VIP:
			return iterateThroughLoadout(rank, s_VIPLoadoutWeapons, VIP_WEAPON_LOADOUTS);
		default:
			Assert(false);
			return 0;
		}
	}

	static int GetTotalLoadoutSize(int classType, int isDev) {
		if (isDev) { return DEV_WEAPON_LOADOUTS; }
		switch (classType) {
		case NEO_CLASS_RECON:
			return RECON_WEAPON_LOADOUTS;
		case NEO_CLASS_ASSAULT:
			return ASSAULT_WEAPON_LOADOUTS;
		case NEO_CLASS_SUPPORT:
			return SUPPORT_WEAPON_LOADOUTS;
		case NEO_CLASS_VIP:
			return VIP_WEAPON_LOADOUTS;
		default:
			return 0;
		}
	}

	static const char* GetLoadoutVguiWeaponName(int classType, int weaponPositionInLoadout, bool isDev)
	{
		if (isDev)
		{
			if (weaponPositionInLoadout < DEV_WEAPON_LOADOUTS) { return s_DevLoadoutWeapons[weaponPositionInLoadout].m_szVguiImage; }
		}
		switch (classType) {
		case NEO_CLASS_RECON:
			if (weaponPositionInLoadout < RECON_WEAPON_LOADOUTS) { return s_ReconLoadoutWeapons[weaponPositionInLoadout].m_szVguiImage; }
			return "";
		case NEO_CLASS_ASSAULT:
			if (weaponPositionInLoadout < ASSAULT_WEAPON_LOADOUTS) { return s_AssaultLoadoutWeapons[weaponPositionInLoadout].m_szVguiImage; }
			return "";
		case NEO_CLASS_SUPPORT:
			if (weaponPositionInLoadout < SUPPORT_WEAPON_LOADOUTS) { return s_SupportLoadoutWeapons[weaponPositionInLoadout].m_szVguiImage; }
			return "";
		case NEO_CLASS_VIP:
			if (weaponPositionInLoadout < VIP_WEAPON_LOADOUTS) { return s_VIPLoadoutWeapons[weaponPositionInLoadout].m_szVguiImage; }
			return "";
		default:
			Assert(false);
			return "";
		}
	}

	static const char* GetLoadoutVguiWeaponNameNo(int classType, int weaponPositionInLoadout, bool isDev)
	{
		if (isDev)
		{
			if (weaponPositionInLoadout < DEV_WEAPON_LOADOUTS) { return s_DevLoadoutWeapons[weaponPositionInLoadout].m_szVguiImageNo; }
		}
		switch (classType) {
		case NEO_CLASS_RECON:
			if (weaponPositionInLoadout < RECON_WEAPON_LOADOUTS) { return s_ReconLoadoutWeapons[weaponPositionInLoadout].m_szVguiImageNo; }
			return "";
		case NEO_CLASS_ASSAULT:
			if (weaponPositionInLoadout < ASSAULT_WEAPON_LOADOUTS) { return s_AssaultLoadoutWeapons[weaponPositionInLoadout].m_szVguiImageNo; }
			return "";
		case NEO_CLASS_SUPPORT:
			if (weaponPositionInLoadout < SUPPORT_WEAPON_LOADOUTS) { return s_SupportLoadoutWeapons[weaponPositionInLoadout].m_szVguiImageNo; }
			return "";
		case NEO_CLASS_VIP:
			if (weaponPositionInLoadout < VIP_WEAPON_LOADOUTS) { return s_VIPLoadoutWeapons[weaponPositionInLoadout].m_szVguiImageNo; }
			return "";
		default:
			Assert(false);
			return "";
		}
	}


	static const char* GetLoadoutWeaponEntityName(int classType, int weaponPositionInLoadout, bool isDev)
	{
		if (isDev)
		{
			if (weaponPositionInLoadout < DEV_WEAPON_LOADOUTS) { return s_DevLoadoutWeapons[weaponPositionInLoadout].m_szWeaponEntityName; }
		}
		switch (classType) {
		case NEO_CLASS_RECON:
			if (weaponPositionInLoadout < RECON_WEAPON_LOADOUTS) { return s_ReconLoadoutWeapons[weaponPositionInLoadout].m_szWeaponEntityName; }
			return "";
		case NEO_CLASS_ASSAULT:
			if (weaponPositionInLoadout < ASSAULT_WEAPON_LOADOUTS) { return s_AssaultLoadoutWeapons[weaponPositionInLoadout].m_szWeaponEntityName; }
			return "";
		case NEO_CLASS_SUPPORT:
			if (weaponPositionInLoadout < SUPPORT_WEAPON_LOADOUTS) { return s_SupportLoadoutWeapons[weaponPositionInLoadout].m_szWeaponEntityName; }
			return "";
		case NEO_CLASS_VIP:
			if (weaponPositionInLoadout < VIP_WEAPON_LOADOUTS) { return s_VIPLoadoutWeapons[weaponPositionInLoadout].m_szWeaponEntityName; }
			return "";
		default:
			Assert(false);
			return "";
		}
	}

private:
	static int iterateThroughLoadout(int rank, CLoadoutWeaponClass* loadout, int loadOutSize)
	{
		int amount = 0;

		for (int i = 0; i < loadOutSize; i++)
		{
			if (loadout[i].m_iWeaponPrice > rank)
			{
				return amount;
			}
			amount++;
		}
		return amount;
	}
};

CLoadoutWeaponClass CNEOWeaponLoadout::s_DevLoadoutWeapons[DEV_WEAPON_LOADOUTS] =
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", -255, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "SRM (silenced)", -255, "loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
	{ "Jitte", -255, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "Jitte (with scope)", -255, "/loadout/loadout_jittes", "loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
	{ "ZR68C", -255, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "ZR68-S (silenced)", -255, "loadout/loadout_zr68s", "loadout/loadout_zr68s_no", "weapon_zr68s", "AMMO_PRI" },
	{ "ZR68-L (accurized)", -255, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
	{ "MX", -255, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
	{ "PZ252", -255, "loadout/loadout_pz", "loadout/loadout_pz_no", "weapon_pz", "AMMO_PRI" },
	{ "Murata Supa-7", -255, "loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_PRI" },
	{ "Mosok", -255, "loadout/loadout_mosok", "loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" }
};

CLoadoutWeaponClass CNEOWeaponLoadout::s_ReconLoadoutWeapons[RECON_WEAPON_LOADOUTS] =
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "Jitte", 0, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "SRM (silenced)", 4, "/loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
	{ "Jitte (with scope)", 4, "/loadout/loadout_jittes", "loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
	{ "ZR68-L (accurized)", 4, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
	{ "ZR68C", 10, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "Murata Supa-7", 20, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
	{ "Mosok Silenced", 20, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" }
};

CLoadoutWeaponClass CNEOWeaponLoadout::s_AssaultLoadoutWeapons[ASSAULT_WEAPON_LOADOUTS] =
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "Jitte", 0, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "ZR68C", 0, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "ZR68-S (silenced)", 0, "loadout/loadout_zr68s", "loadout/loadout_zr68s_no", "weapon_zr68s", "AMMO_PRI" },
	{ "Murata Supa-7", 4, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
	{ "Mosok", 4, "/loadout/loadout_mosok", "/loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
	{ "Mosok Silenced", 10, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" },
	{ "MX", 10, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
	{ "MX Silenced", 10, "/loadout/loadout_mxs", "/loadout/loadout_mxs_no", "weapon_mx_silenced", "AMMO_PRI" },
	{ "AA13", 20, "/loadout/loadout_aa13", "/loadout/loadout_aa13_no", "weapon_aa13", "AMMO_10G_SHELL" },
	{ "SRS", 20, "/loadout/loadout_srs", "/loadout/loadout_srs_no", "weapon_srs", "AMMO_PRI" }
};

CLoadoutWeaponClass CNEOWeaponLoadout::s_SupportLoadoutWeapons[SUPPORT_WEAPON_LOADOUTS] =
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "ZR68C", 0, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "Mosok", 0, "/loadout/loadout_mosok", "/loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
	{ "Murata Supa-7", 0, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
	{ "MX", 4, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
	{ "Mosok Silenced", 4, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" },
	{ "MX Silenced", 10, "/loadout/loadout_mxs", "/loadout/loadout_mxs_no", "weapon_mx_silenced", "AMMO_PRI" },
	{ "PZ252", 20, "loadout/loadout_pz", "loadout/loadout_pz_no", "weapon_pz", "AMMO_PRI" }
};

CLoadoutWeaponClass CNEOWeaponLoadout::s_VIPLoadoutWeapons[VIP_WEAPON_LOADOUTS] =
{
	{ "SMAC", -255, "loadout/loadout_smac", "loadout/loadout_smac_no", "weapon_smac", "AMMO_PRI" },
	{ "MPN", 4, "/loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 4, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "Jitte", 10, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "SRM (silenced)", 10, "/loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
	{ "Jitte (with scope)", 20, "/loadout/loadout_jittes", "/loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
	{ "ZR68C", 20, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "ZR68-L (accurized)", 20, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
	{ "Murata Supa-7", 20, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
};
#endif // NEO_WEAPON_LOADOUT_H