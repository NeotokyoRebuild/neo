//#include "neo_weapon_loadout.h"
//
//class CNeoWeaponLoadout {
//	public:
//		static CLoadoutWeaponClass s_DevLoadoutWeapons[13];
//		static CLoadoutWeaponClass s_ReconLoadoutWeapons[9];
//		static CLoadoutWeaponClass s_AssaultLoadoutWeapons[12];
//		static CLoadoutWeaponClass s_SupportLoadoutWeapons[9];
//	/*
//		static const int i_DevLoadoutWeaponsSize = ARRAYSIZE(s_DevLoadoutWeapons);
//		static const int i_ReconLoadoutWeaponsSize = ARRAYSIZE(s_ReconLoadoutWeapons);
//		static const int i_AssaultLoadoutWeaponsSize = ARRAYSIZE(s_AssaultLoadoutWeapons);
//		static const int i_SupportLoadoutWeaponsSize = ARRAYSIZE(s_SupportLoadoutWeapons);*/
//
//
//		//CNeoWeaponLoadout::CLoadoutWeaponClass(const char* weaponName, int weaponPrice, const char* vguiImage, const char* vguiImageNo, const char* weaponEntityName, const char* ammoType)
//		//{
//		//	m_szWeaponName = weaponName;
//		//	m_iWeaponPrice = weaponPrice;
//		//	m_szVguiImage = vguiImage;
//		//	m_szVguiImageNo = vguiImageNo;
//		//	m_szWeaponEntityName = weaponEntityName;
//		//	m_szAmmoType = ammoType;
//		//}
//
//		int GetNumberOfLoadoutWeapons(int rank, int classType, CLoadoutWeaponClass* pLoadout, int selectedLoadoutSize)
//		{
//			int ammount = 0;
//			int i = 0;
//
//			for (i; i < selectedLoadoutSize; i++)
//			{
//				if (pLoadout[i].m_iWeaponPrice > rank)
//				{
//					return ammount;
//				}
//				ammount++;
//			}
//			return ammount;
//		}
//};
//
//CLoadoutWeaponClass CNeoWeaponLoadout::s_DevLoadoutWeapons[13] = 
//{
//	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
//	{ "SRM", -255, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
//	{ "SRM (silenced)", -255, "loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
//	{ "Jitte", -255, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
//	{ "Jitte (with scope)", -255, "/loadout/loadout_jittes", "loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
//	{ "ZR68C", -255, "/loadout/loadout_zr68c", "weapon_zr68c", "loadout/loadout_zr68c_no", "AMMO_PRI" },
//	{ "ZR68-S (silenced)", -255, "loadout/loadout_zr68s", "loadout/loadout_zr68s_no", "weapon_zr68s", "AMMO_PRI" },
//	{ "ZR68-L (accurized)", -255, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
//	{ "MX", -255, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
//	{ "PZ252", -255, "loadout/loadout_pz", "loadout/loadout_pz_no", "weapon_pz", "AMMO_PRI" },
//	{ "Murata Supa-7", -255, "loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_PRI" },
//	{ "Mosok", -255, "loadout/loadout_mosok", "loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
//	{ "Mosok (with scope)", -255, "loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41l", "AMMO_PRI" }
//};
//
//CLoadoutWeaponClass CNeoWeaponLoadout::s_ReconLoadoutWeapons[9] = 
//{
//	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
//	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
//	{ "Jitte", 0, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
//	{ "SRM (silenced)", 4, "/loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
//	{ "Jitte (with scope)", 4, "/loadout/loadout_jittes", "loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
//	{ "ZR68-L (accurized)", 4, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
//	{ "ZR68C", 10, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
//	{ "Murata Supa-7", 20, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
//	{ "Mosok Silenced", 20, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" }
//};
//
//CLoadoutWeaponClass CNeoWeaponLoadout::s_AssaultLoadoutWeapons[12] =
//{
//	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
//	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
//	{ "Jitte", 0, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
//	{ "ZR68C", 0, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
//	{ "ZR68-S (silenced)", 4, "loadout/loadout_zr68s", "loadout/loadout_zr68s_no", "weapon_zr68s", "AMMO_PRI" },
//	{ "Murata Supa-7", 4, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
//	{ "Mosok", 4, "/loadout/loadout_mosok", "/loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
//	{ "Mosok Silenced", 10, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" },
//	{ "MX", 10, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
//	{ "MX Silenced", 10, "/loadout/loadout_mxs", "/loadout/loadout_mxs_no", "weapon_mx_silenced", "AMMO_PRI" },
//	{ "AA13", 20, "/loadout/loadout_aa13", "/loadout/loadout_aa13_no", "weapon_aa13", "AMMO_10G_SHELL" },
//	{ "SRS", 20, "/loadout/loadout_srs", "/loadout/loadout_srs_no", "weapon_srs", "AMMO_PRI" }
//};
//
//CLoadoutWeaponClass CNeoWeaponLoadout::s_SupportLoadoutWeapons[9] =
//{
//	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
//	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
//	{ "ZR68C", 0, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
//	{ "Mosok", 0, "/loadout/loadout_mosok", "/loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
//	{ "Murata Supa-7", 0, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
//	{ "MX", 4, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
//	{ "Mosok Silenced", 4, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" },
//	{ "MX Silenced", 10, "/loadout/loadout_mxs", "/loadout/loadout_mxs_no", "weapon_mx_silenced", "AMMO_PRI" },
//	{ "PZ252", 20, "loadout/loadout_pz", "loadout/loadout_pz_no", "weapon_pz", "AMMO_PRI" }
//};