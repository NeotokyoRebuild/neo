#include "cbase.h"
#include "neo_root.h"
#include "IOverrideInterface.h"

#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include "neo_version_info.h"
#include "cdll_client_int.h"
#include <steam/steam_api.h>
#include <vgui_avatarimage.h>
#include <IGameUIFuncs.h>
#include "inputsystem/iinputsystem.h"
#include "characterset.h"
#include "materialsystem/materialsystem_config.h"
#include <voice_status.h>
#include <ivoicetweak.h>
#include <c_playerresource.h>

#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

extern ConVar neo_name;
extern ConVar cl_onlysteamnick;
extern ConVar sensitivity;
extern ConVar snd_victory_volume;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// TODO: Gamepad
//   Gamepad enable: joystick 0/1
//   Reverse up-down axis: joy_inverty 0/1
//   Swap sticks on dual-stick controllers: joy_movement_stick 0/1
//   Horizontal sens: joy_yawsensitivity: -0.5 to -7.0
//   Vertical sens: joy_pitchsensitivity: 0.5 to 7.0

using namespace vgui;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule
// the dll and that we don't call Sys_LoadModule over and over again.
static CDllDemandLoader g_GameUIDLL( "GameUI" );

CNeoRoot *g_pNeoRoot = nullptr;
void NeoToggleconsole();

namespace {

inline NeoUI::Context g_uiCtx;
int g_iRowsInScreen;
int g_iAvatar = 64;
int g_iRootSubPanelWide = 600;
int g_iGSIX[GSIW__TOTAL] = {};
constexpr wchar_t WSZ_GAME_TITLE[] = L"neatbkyoc ue";

const wchar_t *QUALITY_LABELS[] = {
	L"Low",
	L"Medium",
	L"High",
	L"Very High",
};

enum QualityEnum
{
	QUALITY_LOW = 0,
	QUALITY_MEDIUM,
	QUALITY_HIGH,
	QUALITY_VERYHIGH,
};

ConCommand neo_toggleconsole("neo_toggleconsole", NeoToggleconsole);
}

void OverrideGameUI()
{
	if (!OverrideUI->GetPanel())
	{
		OverrideUI->Create(0U);
	}

	if (g_pNeoRoot->GetGameUI())
	{
		g_pNeoRoot->GetGameUI()->SetMainMenuOverride(g_pNeoRoot->GetVPanel());
	}
}

extern ConVar neo_fov;
extern ConVar neo_viewmodel_fov_offset;
extern ConVar neo_aim_hold;
extern ConVar cl_autoreload_when_empty;
extern ConVar cl_righthand;
extern ConVar cl_showpos;
extern ConVar cl_showfps;
extern ConVar hud_fastswitch;

enum MultiThreadIdx
{
	THREAD_SINGLE = 0,
	THREAD_MULTI,
};

enum FilteringEnum
{
	FILTERING_BILINEAR = 0,
	FILTERING_TRILINEAR,
	FILTERING_ANISO2X,
	FILTERING_ANISO4X,
	FILTERING_ANISO8X,
	FILTERING_ANISO16X,

	FILTERING__TOTAL,
};

/////////////////
// SERVER BROWSER
/////////////////

enum AnitCheatMode
{
	ANTICHEAT_ANY,
	ANTICHEAT_ON,
	ANTICHEAT_OFF,

	ANTICHEAT__TOTAL,
};

static const wchar_t *ANTICHEAT_LABELS[ANTICHEAT__TOTAL] = {
	L"<Any>", L"On", L"Off"
};

void CNeoServerList::UpdateFilteredList()
{
	// TODO: g_pNeoRoot->m_iSelectedServer Select kept with sorting
	m_filteredServers = m_servers;
	if (m_filteredServers.IsEmpty())
	{
		return;
	}

	// Can't use lamda capture for this, so pass through context
	V_qsort_s(m_filteredServers.Base(), m_filteredServers.Size(), sizeof(gameserveritem_t),
			  [](void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
		const GameServerSortContext gsCtx = *(static_cast<GameServerSortContext *>(vpCtx));
		auto *pGsiLeft = static_cast<const gameserveritem_t *>(vpLeft);
		auto *pGsiRight = static_cast<const gameserveritem_t *>(vpRight);

		// Always set szLeft/szRight to name as fallback
		const char *szLeft = pGsiLeft->GetName();
		const char *szRight = pGsiRight->GetName();
		int iLeft, iRight;
		bool bLeft, bRight;
		switch (gsCtx.col)
		{
		break; case GSIW_LOCKED:
			bLeft = pGsiLeft->m_bPassword;
			bRight = pGsiRight->m_bPassword;
		break; case GSIW_VAC:
			bLeft = pGsiLeft->m_bSecure;
			bRight = pGsiRight->m_bSecure;
		break; case GSIW_MAP:
		{
			const char *szMapLeft = pGsiLeft->m_szMap;
			const char *szMapRight = pGsiRight->m_szMap;
			if (V_strcmp(szMapLeft, szMapRight) != 0)
			{
				szLeft = szMapLeft;
				szRight = szMapRight;
			}
		}
		break; case GSIW_PLAYERS:
			iLeft = pGsiLeft->m_nPlayers;
			iRight = pGsiRight->m_nPlayers;
			if (iLeft == iRight)
			{
				iLeft = pGsiLeft->m_nMaxPlayers;
				iRight = pGsiRight->m_nMaxPlayers;
			}
		break; case GSIW_PING:
			iLeft = pGsiLeft->m_nPing;
			iRight = pGsiRight->m_nPing;
		break; case GSIW_NAME: default: break;
			// no-op, already assigned (default)
		}

		switch (gsCtx.col)
		{
		case GSIW_LOCKED:
		case GSIW_VAC:
			if (bLeft != bRight) return (gsCtx.bDescending) ? bLeft < bRight : bLeft > bRight;
			break;
		case GSIW_PLAYERS:
		case GSIW_PING:
			if (iLeft != iRight) return (gsCtx.bDescending) ? iLeft < iRight : iLeft > iRight;
			break;
		default:
			break;
		}

		return (gsCtx.bDescending) ? V_strcmp(szRight, szLeft) : V_strcmp(szLeft, szRight);
	}, m_pSortCtx);
}

void CNeoServerList::RequestList()
{
	static MatchMakingKeyValuePair_t mmFilters[] = {
		{"gamedir", "neo"},
	};
	MatchMakingKeyValuePair_t *pMMFilters = mmFilters;

	static constexpr HServerListRequest (ISteamMatchmakingServers::*P_FN_REQ[GS__TOTAL])(
				AppId_t, MatchMakingKeyValuePair_t **, uint32, ISteamMatchmakingServerListResponse *) = {
		&ISteamMatchmakingServers::RequestInternetServerList,
		nullptr,
		&ISteamMatchmakingServers::RequestFriendsServerList,
		&ISteamMatchmakingServers::RequestFavoritesServerList,
		&ISteamMatchmakingServers::RequestHistoryServerList,
		&ISteamMatchmakingServers::RequestSpectatorServerList,
	};

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	m_hdlRequest = (m_iType == GS_LAN) ?
				steamMM->RequestLANServerList(engine->GetAppID(), this) :
				(steamMM->*P_FN_REQ[m_iType])(engine->GetAppID(), &pMMFilters, ARRAYSIZE(mmFilters), this);
	m_bSearching = true;
}

// Server has responded ok with updated data
void CNeoServerList::ServerResponded(HServerListRequest hRequest, int iServer)
{
	if (hRequest != m_hdlRequest) return;

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	gameserveritem_t *pServerDetails = steamMM->GetServerDetails(hRequest, iServer);
	if (pServerDetails)
	{
		m_servers.AddToTail(*pServerDetails);
		m_bModified = true;
	}
}

// Server has failed to respond
void CNeoServerList::ServerFailedToRespond(HServerListRequest hRequest, [[maybe_unused]] int iServer)
{
	if (hRequest != m_hdlRequest) return;
}

// A list refresh you had initiated is now 100% completed
void CNeoServerList::RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response)
{
	if (hRequest != m_hdlRequest) return;

	m_bSearching = false;
	if (response == eNoServersListedOnMasterServer && m_servers.IsEmpty())
	{
		m_bModified = true;
	}
}

CNeoServerPlayers::~CNeoServerPlayers()
{
	if (m_hdlQuery != HSERVERQUERY_INVALID)
	{
		steamapicontext->SteamMatchmakingServers()->CancelServerQuery(m_hdlQuery);
	}
}

void CNeoServerPlayers::RequestList(uint32 unIP, uint16 usPort)
{
	auto *steamMM = steamapicontext->SteamMatchmakingServers();
	if (m_hdlQuery != HSERVERQUERY_INVALID)
	{
		steamMM->CancelServerQuery(m_hdlQuery);
	}
	m_hdlQuery = steamMM->PlayerDetails(unIP, usPort, this);
	m_players.RemoveAll();
}

void CNeoServerPlayers::AddPlayerToList(const char *pchName, int nScore, float flTimePlayed)
{
	PlayerInfo playerInfo{
		.iScore = nScore,
		.flTimePlayed = flTimePlayed,
	};
	g_pVGuiLocalize->ConvertANSIToUnicode(pchName, playerInfo.wszName, sizeof(playerInfo.wszName));
	m_players.AddToTail(playerInfo);
}

void CNeoServerPlayers::PlayersFailedToRespond()
{
	// no-op
}

void CNeoServerPlayers::PlayersRefreshComplete()
{
	m_hdlQuery = HSERVERQUERY_INVALID;
}

static const char *DLFILTER_STRMAP[] = {
	"all", "nosounds", "mapsonly", "none"
};

void NeoSettingsInit(NeoSettings *ns)
{
	int iNativeWidth, iNativeHeight;
	gameuifuncs->GetDesktopResolution(iNativeWidth, iNativeHeight);
	static constexpr int DISP_SIZE = sizeof("(#######x#######) (Native)");
	NeoSettings::Video *pVideo = &ns->video;
	gameuifuncs->GetVideoModes(&pVideo->vmList, &pVideo->iVMListSize);
	pVideo->p2WszVmDispList = (wchar_t **)calloc(sizeof(wchar_t *), pVideo->iVMListSize);
	pVideo->p2WszVmDispList[0] = (wchar_t *)calloc(sizeof(wchar_t) * DISP_SIZE, pVideo->iVMListSize);
	for (int i = 0, offset = 0; i < pVideo->iVMListSize; ++i, offset += DISP_SIZE)
	{
		vmode_t *vm = &pVideo->vmList[i];
		swprintf(pVideo->p2WszVmDispList[0] + offset, DISP_SIZE - 1, L"%d x %d%ls",
				 vm->width, vm->height,
				 (iNativeWidth == vm->width && iNativeHeight == vm->height) ? L" (Native)" : L"");
		pVideo->p2WszVmDispList[i] = pVideo->p2WszVmDispList[0] + offset;
	}

	// TODO: Alt/secondary keybind, different way on finding keybind
	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	if (filesystem->ReadFile("scripts/kb_act.lst", nullptr, buf))
	{
		characterset_t breakSet = {};
		breakSet.set[0] = '"';
		NeoSettings::Keys *keys = &ns->keys;
		keys->iBindsSize = 0;

		while (buf.IsValid() && keys->iBindsSize < ARRAYSIZE(keys->vBinds))
		{
			char szFirstCol[64];
			char szRawDispText[64];

			bool bIsOk = false;
			bIsOk = buf.ParseToken(&breakSet, szFirstCol, sizeof(szFirstCol));
			if (!bIsOk) break;
			bIsOk = buf.ParseToken(&breakSet, szRawDispText, sizeof(szRawDispText));
			if (!bIsOk) break;

			if (szFirstCol[0] == '\0') continue;

			wchar_t wszDispText[64];
			if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(szRawDispText))
			{
				V_wcscpy_safe(wszDispText, localizedWszStr);
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(szRawDispText, wszDispText, sizeof(wszDispText));
			}

			const bool bIsBlank = V_strcmp(szFirstCol, "blank") == 0;
			if (bIsBlank && szRawDispText[0] != '=')
			{
				// This is category label
				auto *bind = &keys->vBinds[keys->iBindsSize++];
				bind->szBindingCmd[0] = '\0';
				V_wcscpy_safe(bind->wszDisplayText, wszDispText);
			}
			else if (!bIsBlank)
			{
				// This is a keybind
				auto *bind = &keys->vBinds[keys->iBindsSize++];
				V_strcpy_safe(bind->szBindingCmd, szFirstCol);
				V_wcscpy_safe(bind->wszDisplayText, wszDispText);
			}
		}
	}
}

void NeoSettingsDeinit(NeoSettings *ns)
{
	free(ns->video.p2WszVmDispList[0]);
	free(ns->video.p2WszVmDispList);
}

void NeoSettingsRestore(NeoSettings *ns)
{
	ns->bModified = false;
	NeoSettings::CVR *cvr = &ns->cvr;
	{
		NeoSettings::General *pGeneral = &ns->general;
		g_pVGuiLocalize->ConvertANSIToUnicode(neo_name.GetString(), pGeneral->wszNeoName, sizeof(pGeneral->wszNeoName));
		pGeneral->bOnlySteamNick = cl_onlysteamnick.GetBool();
		pGeneral->iFov = neo_fov.GetInt();
		pGeneral->iViewmodelFov = neo_viewmodel_fov_offset.GetInt();
		pGeneral->bAimHold = neo_aim_hold.GetBool();
		pGeneral->bReloadEmpty = cl_autoreload_when_empty.GetBool();
		pGeneral->bViewmodelRighthand = cl_righthand.GetBool();
		pGeneral->bShowPlayerSprays = !(cvr->cl_player_spray_disable.GetBool()); // Inverse
		pGeneral->bShowPos = cl_showpos.GetBool();
		pGeneral->iShowFps = cl_showfps.GetInt();
		{
			const char *szDlFilter = cvr->cl_download_filter.GetString();
			pGeneral->iDlFilter = 0;
			for (int i = 0; i < ARRAYSIZE(DLFILTER_STRMAP); ++i)
			{
				if (V_strcmp(szDlFilter, DLFILTER_STRMAP[i]) == 0)
				{
					pGeneral->iDlFilter = i;
					break;
				}
			}
		}
	}
	{
		NeoSettings::Keys *pKeys = &ns->keys;
		pKeys->bWeaponFastSwitch = hud_fastswitch.GetBool();
		pKeys->bDeveloperConsole = (gameuifuncs->GetButtonCodeForBind("neo_toggleconsole") > KEY_NONE);
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			auto *bind = &pKeys->vBinds[i];
			bind->bcNext = bind->bcCurrent = gameuifuncs->GetButtonCodeForBind(bind->szBindingCmd);
		}
	}
	{
		NeoSettings::Mouse *pMouse = &ns->mouse;
		pMouse->flSensitivity = sensitivity.GetFloat();
		pMouse->bRawInput = cvr->m_raw_input.GetBool();
		pMouse->bFilter = cvr->m_filter.GetBool();
		pMouse->bReverse = (cvr->pitch.GetFloat() < 0.0f);
		pMouse->bCustomAccel = (cvr->m_customaccel.GetInt() == 3);
		pMouse->flExponent = cvr->m_customaccel_exponent.GetFloat();
	}
	{
		NeoSettings::Audio *pAudio = &ns->audio;

		// Output
		pAudio->flVolMain = cvr->volume.GetFloat();
		pAudio->flVolMusic = cvr->snd_musicvolume.GetFloat();
		pAudio->flVolVictory = snd_victory_volume.GetFloat();
		pAudio->iSoundSetup = 0;
		switch (cvr->snd_surround_speakers.GetInt())
		{
		break; case 0: pAudio->iSoundSetup = 1;
		break; case 2: pAudio->iSoundSetup = 2;
	#ifndef LINUX
		break; case 4: pAudio->iSoundSetup = 3;
		break; case 5: pAudio->iSoundSetup = 4;
		break; case 7: pAudio->iSoundSetup = 5;
	#endif
		break; default: break;
		}
		pAudio->bMuteAudioUnFocus = cvr->snd_mute_losefocus.GetBool();

		// Sound quality:  snd_pitchquality   dsp_slow_cpu
		// High                   1                0
		// Medium                 0                0
		// Low                    0                1
		pAudio->iSoundQuality =	(cvr->snd_pitchquality.GetBool()) 	? QUALITY_HIGH :
								(cvr->dsp_slow_cpu.GetBool()) 		? QUALITY_LOW :
																	  QUALITY_MEDIUM;

		// Input
		pAudio->bVoiceEnabled = cvr->voice_enable.GetBool();
		pAudio->flVolVoiceRecv = cvr->voice_scale.GetFloat();
		pAudio->bMicBoost = (engine->GetVoiceTweakAPI()->GetControlFloat(MicBoost) != 0.0f);
	}
	{
		NeoSettings::Video *pVideo = &ns->video;

		int iScreenWidth, iScreenHeight;
		engine->GetScreenSize(iScreenWidth, iScreenHeight); // Or: g_pMaterialSystem->GetDisplayMode?
		pVideo->iResolution = pVideo->iVMListSize - 1;
		for (int i = 0; i < pVideo->iVMListSize; ++i)
		{
			vmode_t *vm = &pVideo->vmList[i];
			if (vm->width == iScreenWidth && vm->height == iScreenHeight)
			{
				pVideo->iResolution = i;
				break;
			}
		}

		pVideo->iWindow = static_cast<int>(g_pMaterialSystem->GetCurrentConfigForVideoCard().Windowed());
		const int queueMode = cvr->mat_queue_mode.GetInt();
		pVideo->iCoreRendering = (queueMode == -1 || queueMode == 2) ? THREAD_MULTI : THREAD_SINGLE;
		pVideo->iModelDetail = 2 - cvr->r_rootlod.GetInt(); // Inverse, highest = 0, lowest = 2
		pVideo->iTextureDetail = 3 - (cvr->mat_picmip.GetInt() + 1); // Inverse+1, highest = -1, lowest = 2
		pVideo->iShaderDetail = 1 - cvr->mat_reducefillrate.GetInt(); // Inverse, 1 = low, 0 = high
		// Water detail
		//                r_waterforceexpensive        r_waterforcereflectentities
		// Simple:                  0                              0
		// Reflect World:           1                              0
		// Reflect all:             1                              1
		pVideo->iWaterDetail = 	(cvr->r_waterforcereflectentities.GetBool()) 	? QUALITY_HIGH :
								(cvr->r_waterforceexpensive.GetBool()) 			? QUALITY_MEDIUM :
																				  QUALITY_LOW;

		// Shadow detail
		//         r_flashlightdepthtexture     r_shadowrendertotexture
		// Low:              0                            0
		// Medium:           0                            1
		// High:             1                            1
		pVideo->iShadowDetail =	(cvr->r_flashlightdepthtexture.GetBool()) 		? QUALITY_HIGH :
								(cvr->r_shadowrendertotexture.GetBool()) 		? QUALITY_MEDIUM :
																				  QUALITY_LOW;

		pVideo->bColorCorrection = cvr->mat_colorcorrection.GetBool();
		pVideo->iAntiAliasing = (cvr->mat_antialias.GetInt() / 2); // MSAA: Times by 2

		// Filtering mode
		// mat_trilinear: 0 = bilinear, 1 = trilinear (both: mat_forceaniso 1)
		// mat_forceaniso: Antisotropic 2x/4x/8x/16x (all aniso: mat_trilinear 0)
		int filterIdx = 0;
		if (cvr->mat_forceaniso.GetInt() < 2)
		{
			filterIdx = cvr->mat_trilinear.GetBool() ? FILTERING_TRILINEAR : FILTERING_BILINEAR;
		}
		else
		{
			switch (cvr->mat_forceaniso.GetInt())
			{
			break; case 2: filterIdx = FILTERING_ANISO2X;
			break; case 4: filterIdx = FILTERING_ANISO4X;
			break; case 8: filterIdx = FILTERING_ANISO8X;
			break; case 16: filterIdx = FILTERING_ANISO16X;
			break; default: filterIdx = FILTERING_ANISO4X; // Some invalid number, just set to 4X (idx 3)
			}
		}
		pVideo->iFilteringMode = filterIdx;
		pVideo->bVSync = cvr->mat_vsync.GetBool();
		pVideo->bMotionBlur = cvr->mat_motion_blur_enabled.GetBool();
		pVideo->iHDR = cvr->mat_hdr_level.GetInt();
		pVideo->flGamma = cvr->mat_monitorgamma.GetFloat();
	}
}

void NeoSettingsSave(const NeoSettings *ns)
{
	const_cast<NeoSettings *>(ns)->bModified = false;
	auto *cvr = const_cast<NeoSettings::CVR *>(&ns->cvr);
	{
		const NeoSettings::General *pGeneral = &ns->general;
		char neoNameText[sizeof(pGeneral->wszNeoName) / sizeof(wchar_t)];
		g_pVGuiLocalize->ConvertUnicodeToANSI(pGeneral->wszNeoName, neoNameText, sizeof(neoNameText));
		neo_name.SetValue(neoNameText);
		cl_onlysteamnick.SetValue(pGeneral->bOnlySteamNick);
		neo_fov.SetValue(pGeneral->iFov);
		neo_viewmodel_fov_offset.SetValue(pGeneral->iViewmodelFov);
		neo_aim_hold.SetValue(pGeneral->bAimHold);
		cl_autoreload_when_empty.SetValue(pGeneral->bReloadEmpty);
		cl_righthand.SetValue(pGeneral->bViewmodelRighthand);
		cvr->cl_player_spray_disable.SetValue(!pGeneral->bShowPlayerSprays); // Inverse
		cl_showpos.SetValue(pGeneral->bShowPos);
		cl_showfps.SetValue(pGeneral->iShowFps);
		cvr->cl_download_filter.SetValue(DLFILTER_STRMAP[pGeneral->iDlFilter]);
	}
	{
		const NeoSettings::Keys *pKeys = &ns->keys;
		hud_fastswitch.SetValue(pKeys->bWeaponFastSwitch);
		{
			char cmdStr[128];
			V_sprintf_safe(cmdStr, "unbind \"`\"\n");
			engine->ClientCmd_Unrestricted(cmdStr);

			if (pKeys->bDeveloperConsole)
			{
				V_sprintf_safe(cmdStr, "bind \"`\" \"neo_toggleconsole\"\n");
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			const auto *bind = &pKeys->vBinds[i];
			if (bind->szBindingCmd[0] != '\0')
			{
				char cmdStr[128];
				const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcCurrent);
				V_sprintf_safe(cmdStr, "unbind \"%s\"\n", bindBtnName);
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			const auto *bind = &pKeys->vBinds[i];
			if (bind->szBindingCmd[0] != '\0')
			{
				char cmdStr[128];
				const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcNext);
				V_sprintf_safe(cmdStr, "bind \"%s\" \"%s\"\n", bindBtnName, bind->szBindingCmd);
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
	}
	{
		const NeoSettings::Mouse *pMouse = &ns->mouse;
		sensitivity.SetValue(pMouse->flSensitivity);
		cvr->m_raw_input.SetValue(pMouse->bRawInput);
		cvr->m_filter.SetValue(pMouse->bFilter);
		const float absPitch = abs(cvr->pitch.GetFloat());
		cvr->pitch.SetValue(pMouse->bReverse ? -absPitch : absPitch);
		cvr->m_customaccel.SetValue(pMouse->bCustomAccel ? 3 : 0);
		cvr->m_customaccel_exponent.SetValue(pMouse->flExponent);
	}
	{
		const NeoSettings::Audio *pAudio = &ns->audio;

		static constexpr int SURROUND_RE_MAP[] = {
			-1, 0, 2,
#ifndef LINUX
			4, 5, 7
#endif
		};

		// Output
		cvr->volume.SetValue(pAudio->flVolMain);
		cvr->snd_musicvolume.SetValue(pAudio->flVolMusic);
		snd_victory_volume.SetValue(pAudio->flVolVictory);
		cvr->snd_surround_speakers.SetValue(SURROUND_RE_MAP[pAudio->iSoundSetup]);
		cvr->snd_mute_losefocus.SetValue(pAudio->bMuteAudioUnFocus);
		cvr->snd_pitchquality.SetValue(pAudio->iSoundQuality == QUALITY_HIGH);
		cvr->dsp_slow_cpu.SetValue(pAudio->iSoundQuality == QUALITY_LOW);

		// Input
		cvr->voice_enable.SetValue(pAudio->bVoiceEnabled);
		cvr->voice_scale.SetValue(pAudio->flVolVoiceRecv);
		engine->GetVoiceTweakAPI()->SetControlFloat(MicBoost, static_cast<float>(pAudio->bMicBoost));
	}
	{
		const NeoSettings::Video *pVideo = &ns->video;

		const int resIdx = pVideo->iResolution;
		if (resIdx >= 0 && resIdx < pVideo->iVMListSize)
		{
			// mat_setvideomode [width] [height] [mode] | mode: 0 = fullscreen, 1 = windowed
			vmode_t *vm = &pVideo->vmList[resIdx];
			char cmdStr[128];
			V_sprintf_safe(cmdStr, "mat_setvideomode %d %d %d", vm->width, vm->height, pVideo->iWindow);
			engine->ClientCmd_Unrestricted(cmdStr);
		}
		cvr->mat_queue_mode.SetValue((pVideo->iCoreRendering == THREAD_MULTI) ? 2 : 0);
		cvr->r_rootlod.SetValue(2 - pVideo->iModelDetail);
		cvr->mat_picmip.SetValue(2 - pVideo->iTextureDetail);
		cvr->mat_reducefillrate.SetValue(1 - pVideo->iShaderDetail);
		cvr->r_waterforceexpensive.SetValue(pVideo->iWaterDetail >= QUALITY_MEDIUM);
		cvr->r_waterforcereflectentities.SetValue(pVideo->iWaterDetail == QUALITY_HIGH);
		cvr->r_shadowrendertotexture.SetValue(pVideo->iShadowDetail >= QUALITY_MEDIUM);
		cvr->r_flashlightdepthtexture.SetValue(pVideo->iShadowDetail == QUALITY_HIGH);
		cvr->mat_colorcorrection.SetValue(pVideo->bColorCorrection);
		cvr->mat_antialias.SetValue(pVideo->iAntiAliasing * 2);
		cvr->mat_trilinear.SetValue(pVideo->iFilteringMode == FILTERING_TRILINEAR);
		static constexpr int ANISO_MAP[FILTERING__TOTAL] = {
			1, 1, 2, 4, 8, 16
		};
		cvr->mat_forceaniso.SetValue(ANISO_MAP[pVideo->iFilteringMode]);
		cvr->mat_vsync.SetValue(pVideo->bVSync);
		cvr->mat_motion_blur_enabled.SetValue(pVideo->bMotionBlur);
		cvr->mat_hdr_level.SetValue(pVideo->iHDR);
		cvr->mat_monitorgamma.SetValue(pVideo->flGamma);
	}
}

static const wchar_t *DLFILTER_LABELS[] = {
	L"Allow all custom files from server",
	L"Do not download custom sounds",
	L"Only allow map files",
	L"Do not download any custom files",
};
static const wchar_t *SHOWFPS_LABELS[] = { L"Disabled", L"Enabled (FPS)", L"Enabled (Smooth FPS)", };
static_assert(ARRAYSIZE(DLFILTER_STRMAP) == ARRAYSIZE(DLFILTER_LABELS));

void NeoSettings_General(NeoSettings *ns)
{
	NeoSettings::General *pGeneral = &ns->general;
	NeoUI::TextEdit(L"Name", pGeneral->wszNeoName, SZWSZ_LEN(pGeneral->wszNeoName));
	NeoUI::RingBoxBool(L"Show only steam name", &pGeneral->bOnlySteamNick);
	wchar_t wszDisplayName[128];
	const bool bShowSteamNick = pGeneral->bOnlySteamNick || pGeneral->wszNeoName[0] == '\0';
	(bShowSteamNick) ? V_swprintf_safe(wszDisplayName, L"Display name: %s", steamapicontext->SteamFriends()->GetPersonaName())
					 : V_swprintf_safe(wszDisplayName, L"Display name: %ls", pGeneral->wszNeoName);
	NeoUI::Label(wszDisplayName);
	NeoUI::SliderInt(L"FOV", &pGeneral->iFov, 75, 110);
	NeoUI::SliderInt(L"Viewmodel FOV Offset", &pGeneral->iViewmodelFov, -20, 40);
	NeoUI::RingBoxBool(L"Aim hold", &pGeneral->bAimHold);
	NeoUI::RingBoxBool(L"Reload empty", &pGeneral->bReloadEmpty);
	NeoUI::RingBoxBool(L"Right hand viewmodel", &pGeneral->bViewmodelRighthand);
	NeoUI::RingBoxBool(L"Show player spray", &pGeneral->bShowPlayerSprays);
	NeoUI::RingBoxBool(L"Show position", &pGeneral->bShowPos);
	NeoUI::RingBox(L"Show FPS", SHOWFPS_LABELS, ARRAYSIZE(SHOWFPS_LABELS), &pGeneral->iShowFps);
	NeoUI::RingBox(L"Download filter", DLFILTER_LABELS, ARRAYSIZE(DLFILTER_LABELS), &pGeneral->iDlFilter);
}

void NeoSettings_Keys(NeoSettings *ns)
{
	NeoSettings::Keys *pKeys = &ns->keys;
	NeoUI::RingBoxBool(L"Weapon fastswitch", &pKeys->bWeaponFastSwitch);
	NeoUI::RingBoxBool(L"Developer console", &pKeys->bDeveloperConsole);
	g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
	for (int i = 0; i < pKeys->iBindsSize; ++i)
	{
		const auto &bind = pKeys->vBinds[i];
		if (bind.szBindingCmd[0] == '\0')
		{
			NeoUI::Label(bind.wszDisplayText);
		}
		else
		{
			wchar_t wszBindBtnName[64];
			const char *szBindBtnName = g_pInputSystem->ButtonCodeToString(bind.bcNext);
			g_pVGuiLocalize->ConvertANSIToUnicode(szBindBtnName, wszBindBtnName, sizeof(wszBindBtnName));
			if (NeoUI::Button(bind.wszDisplayText, wszBindBtnName).bPressed)
			{
				ns->iNextBinding = i;
			}
		}
	}
}

void NeoSettings_Mouse(NeoSettings *ns)
{
	NeoSettings::Mouse *pMouse = &ns->mouse;
	NeoUI::Slider(L"Sensitivity", &pMouse->flSensitivity, 0.1f, 10.0f, 2, 0.25f);
	NeoUI::RingBoxBool(L"Raw input", &pMouse->bRawInput);
	NeoUI::RingBoxBool(L"Mouse Filter", &pMouse->bFilter);
	NeoUI::RingBoxBool(L"Mouse Reverse", &pMouse->bReverse);
	NeoUI::RingBoxBool(L"Custom Acceleration", &pMouse->bCustomAccel);
	NeoUI::Slider(L"Exponent", &pMouse->flExponent, 1.0f, 1.4f, 2, 0.1f);
}

static const wchar_t *SPEAKER_CFG_LABELS[] = {
	L"Auto", L"Headphones", L"2 Speakers (Stereo)",
#ifndef LINUX
	L"4 Speakers (Quadraphonic)", L"5.1 Surround", L"7.1 Surround",
#endif
};

void NeoSettings_Audio(NeoSettings *ns)
{
	NeoSettings::Audio *pAudio = &ns->audio;
	NeoUI::Slider(L"Main Volume", &pAudio->flVolMain, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Music Volume", &pAudio->flVolMusic, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Victory Volume", &pAudio->flVolVictory, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::RingBox(L"Sound Setup", SPEAKER_CFG_LABELS, ARRAYSIZE(SPEAKER_CFG_LABELS), &pAudio->iSoundSetup);
	NeoUI::RingBox(L"Sound Quality", QUALITY_LABELS, 3, &pAudio->iSoundQuality);
	NeoUI::RingBoxBool(L"Mute Audio on un-focus", &pAudio->bMuteAudioUnFocus);
	NeoUI::RingBoxBool(L"Voice Enabled", &pAudio->bVoiceEnabled);
	NeoUI::Slider(L"Voice Receive", &pAudio->flVolVoiceRecv, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::RingBoxBool(L"Microphone Boost", &pAudio->bMicBoost);
	IVoiceTweak_s *pVoiceTweak = engine->GetVoiceTweakAPI();
	const bool bTweaking = pVoiceTweak->IsStillTweaking();
	if (NeoUI::Button(L"Microphone Tester",
					  bTweaking ? L"Stop testing" : L"Start testing").bPressed)
	{
		bTweaking ? pVoiceTweak->EndVoiceTweakMode() : (void)pVoiceTweak->StartVoiceTweakMode();
	}
	if (bTweaking && g_uiCtx.eMode == NeoUI::MODE_PAINT)
	{
		const float flSpeakingVol = pVoiceTweak->GetControlFloat(SpeakingVolume);
		surface()->DrawSetColor(COLOR_NEOPANELMICTEST);
		NeoUI::GCtxDrawFilledRectXtoX(0, flSpeakingVol * g_uiCtx.dPanel.wide);
		g_uiCtx.iLayoutY += g_uiCtx.iRowTall;
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
	}
}

static const wchar_t *WINDOW_MODE[] = { L"Fullscreen", L"Windowed", };
static const wchar_t *QUEUE_MODE[] = { L"Single", L"Multi", };
static const wchar_t *QUALITY2_LABELS[] = { L"Low", L"High", };
static const wchar_t *FILTERING_LABELS[FILTERING__TOTAL] = {
	L"Bilinear", L"Trilinear", L"Anisotropic 2X", L"Anisotropic 4X", L"Anisotropic 8X", L"Anisotropic 16X",
};
static const wchar_t *HDR_LABELS[] = { L"None", L"Bloom", L"Full", };
static const wchar_t *WATER_LABELS[] = { L"Simple Reflect", L"Reflect World", L"Reflect All", };
static const wchar_t *MSAA_LABELS[] = { L"None", L"2x MSAA", L"4x MSAA", L"6x MSAA", L"8x MSAA", };

void NeoSettings_Video(NeoSettings *ns)
{
	NeoSettings::Video *pVideo = &ns->video;
	NeoUI::RingBox(L"Resolution", const_cast<const wchar_t **>(pVideo->p2WszVmDispList), pVideo->iVMListSize, &pVideo->iResolution);
	NeoUI::RingBox(L"Window", WINDOW_MODE, ARRAYSIZE(WINDOW_MODE), &pVideo->iWindow);
	NeoUI::RingBox(L"Core Rendering", QUEUE_MODE, ARRAYSIZE(QUEUE_MODE), &pVideo->iCoreRendering);
	NeoUI::RingBox(L"Model detail", QUALITY_LABELS, 3, &pVideo->iModelDetail);
	NeoUI::RingBox(L"Texture detail", QUALITY_LABELS, 4, &pVideo->iTextureDetail);
	NeoUI::RingBox(L"Shader detail", QUALITY2_LABELS, 2, &pVideo->iShaderDetail);
	NeoUI::RingBox(L"Water detail", WATER_LABELS, ARRAYSIZE(WATER_LABELS), &pVideo->iWaterDetail);
	NeoUI::RingBox(L"Shadow detail", QUALITY_LABELS, 3, &pVideo->iShadowDetail);
	NeoUI::RingBoxBool(L"Color correction", &pVideo->bColorCorrection);
	NeoUI::RingBox(L"Anti-aliasing", MSAA_LABELS, ARRAYSIZE(MSAA_LABELS), &pVideo->iAntiAliasing);
	NeoUI::RingBox(L"Filtering mode", FILTERING_LABELS, FILTERING__TOTAL, &pVideo->iFilteringMode);
	NeoUI::RingBoxBool(L"V-Sync", &pVideo->bVSync);
	NeoUI::RingBoxBool(L"Motion blur", &pVideo->bMotionBlur);
	NeoUI::RingBox(L"HDR", HDR_LABELS, ARRAYSIZE(HDR_LABELS), &pVideo->iHDR);
	NeoUI::Slider(L"Gamma", &pVideo->flGamma, 1.6, 2.6, 2, 0.1f);
}

///////
// MAIN ROOT PANEL
///////

CNeoRootInput::CNeoRootInput(CNeoRoot *rootPanel)
	: Panel(rootPanel, "NeoRootPanelInput")
	, m_pNeoRoot(rootPanel)
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(false);
	SetVisible(true);
	SetEnabled(true);
	PerformLayout();
}

void CNeoRootInput::PerformLayout()
{
	SetPos(0, 0);
	SetSize(1, 1);
	SetBgColor(COLOR_TRANSPARENT);
	SetFgColor(COLOR_TRANSPARENT);
}

void CNeoRootInput::OnKeyCodeTyped(vgui::KeyCode code)
{
	m_pNeoRoot->OnRelayedKeyCodeTyped(code);
}

void CNeoRootInput::OnKeyTyped(wchar_t unichar)
{
	m_pNeoRoot->OnRelayedKeyTyped(unichar);
}

void CNeoRootInput::OnThink()
{
	ButtonCode_t code;
	if (engine->CheckDoneKeyTrapping(code))
	{
		if (code != KEY_ESCAPE)
		{
			m_pNeoRoot->m_ns.keys.vBinds[m_pNeoRoot->m_iBindingIdx].bcNext =
					(code == KEY_DELETE) ? BUTTON_CODE_NONE : code;
			m_pNeoRoot->m_ns.bModified = true;
		}
		m_pNeoRoot->m_wszBindingText[0] = '\0';
		m_pNeoRoot->m_iBindingIdx = -1;
		m_pNeoRoot->m_state = STATE_SETTINGS;
	}
}

constexpr WidgetInfo BTNS_INFO[BTNS_TOTAL] = {
	{ "#GameUI_GameMenu_ResumeGame", "ResumeGame", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_FindServers", nullptr, STATE_SERVERBROWSER, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_CreateServer", nullptr, STATE_NEWGAME, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Disconnect", "Disconnect", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_PlayerList", nullptr, STATE_PLAYERLIST, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_Options", nullptr, STATE_SETTINGS, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Quit", nullptr, STATE_QUIT, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
};

CNeoRoot::CNeoRoot(VPANEL parent)
	: EditablePanel(nullptr, "NeoRootPanel")
	, m_panelCaptureInput(new CNeoRootInput(this))
{
	SetParent(parent);
	g_pNeoRoot = this;
	LoadGameUI();
	SetVisible(true);
	SetProportional(false);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(neoscheme);

	for (int i = 0; i < BTNS_TOTAL; ++i)
	{
		const char *label = BTNS_INFO[i].label;
		if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(label))
		{
			V_wcsncpy(m_wszDispBtnTexts[i], localizedWszStr, sizeof(m_wszDispBtnTexts[i]));
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(label, m_wszDispBtnTexts[i], sizeof(m_wszDispBtnTexts[i]));
		}
		m_iWszDispBtnTextsSizes[i] = V_wcslen(m_wszDispBtnTexts[i]);
	}

	NeoSettingsInit(&m_ns);
	{
		// Initialize map list
		FileFindHandle_t findHdl;
		for (const char *pszFilename = filesystem->FindFirst("maps/*.bsp", &findHdl);
			 pszFilename;
			 pszFilename = filesystem->FindNext(findHdl))
		{
			// Sanity check: In-case somehow someone named a directory as *.bsp in here
			if (!filesystem->FindIsDirectory(findHdl))
			{
				MapInfo mapInfo;
				int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(pszFilename, mapInfo.wszName, sizeof(mapInfo.wszName));
				iSize -= sizeof(".bsp");
				mapInfo.wszName[iSize] = '\0';
				m_vWszMaps.AddToTail(mapInfo);
			}
		}
		filesystem->FindClose(findHdl);
	}
	for (int i = 0; i < GS__TOTAL; ++i)
	{
		m_serverBrowser[i].m_iType = static_cast<GameServerType>(i);
		m_serverBrowser[i].m_pSortCtx = &m_sortCtx;
	}

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	UpdateControls();
	ivgui()->AddTickSignal(GetVPanel(), 200);
	ListenForGameEvent("server_spawn");
	ListenForGameEvent("game_newmap");

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(neoscheme);
	ApplySchemeSettings(pScheme);
}

CNeoRoot::~CNeoRoot()
{
	m_panelCaptureInput->DeletePanel();
	if (m_avImage) delete m_avImage;
	NeoSettingsDeinit(&m_ns);

	m_gameui = nullptr;
	g_GameUIDLL.Unload();
}

IGameUI *CNeoRoot::GetGameUI()
{
	if (!m_gameui && !LoadGameUI()) return nullptr;
	return m_gameui;
}

void CNeoRoot::UpdateControls()
{

	if (m_state == STATE_ROOT)
	{
		auto hdlFont = g_uiCtx.fonts[NeoUI::FONT_LOGO].hdl;
		surface()->DrawSetTextFont(hdlFont);
		surface()->GetTextSize(hdlFont, WSZ_GAME_TITLE, m_iTitleWidth, m_iTitleHeight);
	}
	g_uiCtx.iActiveDirection = 0;
	g_uiCtx.iActive = NeoUI::FOCUSOFF_NUM;
	g_uiCtx.iActiveSection = -1;
	V_memset(g_uiCtx.iYOffset, 0, sizeof(g_uiCtx.iYOffset));
	m_ns.bBack = false;
	RequestFocus();
	m_panelCaptureInput->RequestFocus();
	InvalidateLayout();
}

bool CNeoRoot::LoadGameUI()
{
	if (!m_gameui)
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if (!gameUIFactory) return false;

		m_gameui = reinterpret_cast<IGameUI *>(gameUIFactory(GAMEUI_INTERFACE_VERSION, nullptr));
		if (!m_gameui) return false;
	}
	return true;
}

void CNeoRoot::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// NEO TODO (nullsystem): If we're to provide color scheme controls:
	// LoadControlSettings("resource/NeoRootScheme.res");

	// Resize the panel to the screen size
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char *FONT_NAMES[NeoUI::FONT__TOTAL] = {
		"NHudOCRSmallNoAdditive", "NHudOCR", "ClientTitleFont"
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		g_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	// In 1080p, g_uiCtx.iRowTall == 40, g_uiCtx.iMarginX = 10, g_iAvatar = 64,
	// other resolution scales up/down from it
	g_uiCtx.iRowTall = tall / 27;
	g_iRowsInScreen = (tall * 0.85f) / g_uiCtx.iRowTall;
	g_uiCtx.iMarginX = wide / 192;
	g_uiCtx.iMarginY = tall / 108;
	g_iAvatar = wide / 30;
	const float flWide = static_cast<float>(wide);
	float flWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (flWideAs43 > flWide) flWideAs43 = flWide;
	g_iRootSubPanelWide = static_cast<int>(flWideAs43 * 0.9f);

	constexpr int PARTITION = GSIW__TOTAL * 4;
	const int iSubDiv = g_iRootSubPanelWide / PARTITION;
	g_iGSIX[GSIW_LOCKED] = iSubDiv * 2;
	g_iGSIX[GSIW_VAC] = iSubDiv * 2;
	g_iGSIX[GSIW_NAME] = iSubDiv * 10;
	g_iGSIX[GSIW_MAP] = iSubDiv * 5;
	g_iGSIX[GSIW_PLAYERS] = iSubDiv * 3;
	g_iGSIX[GSIW_PING] = iSubDiv * 2;

	UpdateControls();
}

void CNeoRoot::Paint()
{
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNeoRoot::OnMousePressed(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEPRESSED);
}

void CNeoRoot::OnMouseDoublePressed(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEDOUBLEPRESSED);
}

void CNeoRoot::OnMouseWheeled(int delta)
{
	g_uiCtx.eCode = (delta > 0) ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
	OnMainLoop(NeoUI::MODE_MOUSEWHEELED);
}

void CNeoRoot::OnCursorMoved(int x, int y)
{
	g_uiCtx.iMouseAbsX = x;
	g_uiCtx.iMouseAbsY = y;
	OnMainLoop(NeoUI::MODE_MOUSEMOVED);
}

void CNeoRoot::OnTick()
{
	if (m_state == STATE_SERVERBROWSER)
	{
		if (m_bSBFiltModified)
		{
			// Pass modified over to the tabs so it doesn't trigger
			// the filter refresh immeditely
			for (int i = 0; i < GS__TOTAL; ++i)
			{
				m_serverBrowser[i].m_bModified = true;
			}
			m_bSBFiltModified = false;
		}

		auto *pSbTab = &m_serverBrowser[m_iServerBrowserTab];
		if (pSbTab->m_bModified)
		{
			pSbTab->UpdateFilteredList();
			pSbTab->m_bModified = false;
		}
	}
}

void CNeoRoot::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (Q_strcmp(type, "server_spawn") == 0)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(event->GetString("hostname"), m_wszHostname, sizeof(m_wszHostname));
	}
	else if (Q_strcmp(type, "game_newmap") == 0)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(event->GetString("mapname"), m_wszMap, sizeof(m_wszMap));
	}
}

void CNeoRoot::OnRelayedKeyCodeTyped(vgui::KeyCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_KEYPRESSED);
}

void CNeoRoot::OnRelayedKeyTyped(wchar_t unichar)
{
	g_uiCtx.unichar = unichar;
	OnMainLoop(NeoUI::MODE_KEYTYPED);
}

void CNeoRoot::OnMainLoop(const NeoUI::Mode eMode)
{
	int wide, tall;
	GetSize(wide, tall);

	const RootState ePrevState = m_state;

	if (eMode == NeoUI::MODE_PAINT)
	{
		// Draw version info (bottom left corner) - Always
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
		int textWidth, textHeight;
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl);
		surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl, BUILD_DISPLAY, textWidth, textHeight);

		surface()->DrawSetTextPos(g_uiCtx.iMarginX, tall - textHeight - g_uiCtx.iMarginY);
		surface()->DrawPrintText(BUILD_DISPLAY, *BUILD_DISPLAY_SIZE);
	}

	static constexpr void (CNeoRoot::*P_FN_MAIN_LOOP[STATE__TOTAL])(const MainLoopParam param) = {
		&CNeoRoot::MainLoopRoot,			// STATE_ROOT
		&CNeoRoot::MainLoopSettings,		// STATE_SETTINGS
		&CNeoRoot::MainLoopNewGame,			// STATE_NEWGAME
		&CNeoRoot::MainLoopServerBrowser,	// STATE_SERVERBROWSER

		&CNeoRoot::MainLoopMapList,			// STATE_MAPLIST
		&CNeoRoot::MainLoopServerDetails,	// STATE_SERVERDETAILS
		&CNeoRoot::MainLoopPlayerList,		// STATE_PLAYERLIST

		&CNeoRoot::MainLoopPopup,			// STATE_KEYCAPTURE
		&CNeoRoot::MainLoopPopup,			// STATE_CONFIRMSETTINGS
		&CNeoRoot::MainLoopPopup,			// STATE_QUIT
		&CNeoRoot::MainLoopPopup,			// STATE_SERVERPASSWORD
	};
	(this->*P_FN_MAIN_LOOP[m_state])(MainLoopParam{.eMode = eMode, .wide = wide, .tall = tall});

	if (m_state != ePrevState)
	{
		UpdateControls();
	}
}

void CNeoRoot::MainLoopRoot(const MainLoopParam param)
{
	const int yTopPos = param.tall / 2 - ((g_uiCtx.iRowTall * BTNS_TOTAL) / 2);
	g_uiCtx.dPanel.wide = m_iTitleWidth + (2 * g_uiCtx.iMarginX);
	g_uiCtx.dPanel.tall = param.tall - yTopPos;
	g_uiCtx.dPanel.x = (param.wide / 4) - (g_uiCtx.dPanel.wide / 2);
	g_uiCtx.dPanel.y = yTopPos;
	g_uiCtx.bgColor = COLOR_TRANSPARENT;

	NeoUI::BeginContext(&g_uiCtx, param.eMode, nullptr, "CtxRoot");
	NeoUI::BeginSection(true);
	{
		const int iFlagToMatch = engine->IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
		for (int i = 0; i < BTNS_TOTAL; ++i)
		{
			const auto btnInfo = BTNS_INFO[i];
			if (btnInfo.flags & iFlagToMatch)
			{
				const auto retBtn = NeoUI::Button(m_wszDispBtnTexts[i]);
				if (retBtn.bPressed || (i == MMBTN_QUIT && !engine->IsInGame() && NeoUI::Bind(KEY_ESCAPE)))
				{
					surface()->PlaySound("ui/buttonclickrelease.wav");
					if (btnInfo.gamemenucommand)
					{
						m_state = STATE_ROOT;
						GetGameUI()->SendMainMenuCommand(btnInfo.gamemenucommand);
					}
					else if (btnInfo.nextState < STATE__TOTAL)
					{
						m_state = btnInfo.nextState;
						if (m_state == STATE_SETTINGS)
						{
							NeoSettingsRestore(&m_ns);
						}
					}
				}
				if (retBtn.bMouseHover && i != m_iHoverBtn)
				{
					// Sound rollover feedback
					surface()->PlaySound("ui/buttonrollover.wav");
					m_iHoverBtn = i;
				}
			}
		}
	}
	NeoUI::EndSection();

	const int iBtnPlaceXMid = (param.wide / 4);
	const int iRightXPos = iBtnPlaceXMid + (m_iBtnWide / 2) + g_uiCtx.iMarginX;
	int iRightSideYStart = yTopPos;

	if (param.eMode == NeoUI::MODE_PAINT)
	{
		// Draw title
		m_iBtnWide = m_iTitleWidth + (2 * g_uiCtx.iMarginX);

		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_LOGO].hdl);
		surface()->DrawSetTextColor(COLOR_NEOTITLE);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth / 2), yTopPos - m_iTitleHeight);
		surface()->DrawPrintText(WSZ_GAME_TITLE, SZWSZ_LEN(WSZ_GAME_TITLE));
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl);

		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
		ISteamUser *steamUser = steamapicontext->SteamUser();
		ISteamFriends *steamFriends = steamapicontext->SteamFriends();
		if (steamUser && steamFriends)
		{
			const int iSteamPlaceXStart = iRightXPos;

			// Draw player info (top left corner)
			const CSteamID steamID = steamUser->GetSteamID();
			if (!m_avImage)
			{
				m_avImage = new CAvatarImage;
				m_avImage->SetAvatarSteamID(steamID, k_EAvatarSize64x64);
			}
			m_avImage->SetPos(iSteamPlaceXStart + g_uiCtx.iMarginX, iRightSideYStart + g_uiCtx.iMarginY);
			m_avImage->SetSize(g_iAvatar, g_iAvatar);
			m_avImage->Paint();

			char szTextBuf[512] = {};
			const char *szSteamName = steamFriends->GetPersonaName();
			const char *szNeoName = neo_name.GetString();
			const bool bUseNeoName = (szNeoName && szNeoName[0] != '\0' && !cl_onlysteamnick.GetBool());
			V_sprintf_safe(szTextBuf, "%s", (bUseNeoName) ? szNeoName : szSteamName);

			wchar_t wszTextBuf[512] = {};
			g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

			int iMainTextWidth, iMainTextHeight;
			surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl);
			surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl, wszTextBuf, iMainTextWidth, iMainTextHeight);

			const int iMainTextStartPosX = g_uiCtx.iMarginX + g_iAvatar + g_uiCtx.iMarginX;
			surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX, iRightSideYStart + g_uiCtx.iMarginY);
			surface()->DrawPrintText(wszTextBuf, V_strlen(szTextBuf));

			if (bUseNeoName)
			{
				V_sprintf_safe(szTextBuf, "(Steam name: %s)", szSteamName);
				g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

				int iSteamSubTextWidth, iSteamSubTextHeight;
				surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl);
				surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl, wszTextBuf, iSteamSubTextWidth, iSteamSubTextHeight);

				const int iRightOfNicknameXPos = iSteamPlaceXStart + iMainTextStartPosX + iMainTextWidth + g_uiCtx.iMarginX;
				// If we have space on the right, set it, otherwise on top of nickname
				if ((iRightOfNicknameXPos + iSteamSubTextWidth) < param.wide)
				{
					surface()->DrawSetTextPos(iRightOfNicknameXPos, iRightSideYStart + g_uiCtx.iMarginY);
				}
				else
				{
					surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
											  iRightSideYStart - iSteamSubTextHeight);
				}
				surface()->DrawPrintText(wszTextBuf, V_strlen(szTextBuf));
			}

			static constexpr const wchar_t *WSZ_PERSONA_STATES[k_EPersonaStateMax] = {
				L"Offline", L"Online", L"Busy", L"Away", L"Snooze", L"Trading", L"Looking to play"
			};
			const auto eCurStatus = steamFriends->GetPersonaState();
			int iStatusTall = 0;
			if (eCurStatus != k_EPersonaStateMax)
			{
				const wchar_t *wszState = WSZ_PERSONA_STATES[static_cast<int>(eCurStatus)];
				const int iStatusTextStartPosY = g_uiCtx.iMarginY + iMainTextHeight + g_uiCtx.iMarginY;

				[[maybe_unused]] int iStatusWide;
				surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl);
				surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTSMALL].hdl, wszState, iStatusWide, iStatusTall);
				surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
										  iRightSideYStart + iStatusTextStartPosY);
				surface()->DrawPrintText(wszState, V_wcslen(wszState));
			}

			// Put the news title in either from avatar or text end Y position
			const int iTextTotalTall = iMainTextHeight + iStatusTall;
			iRightSideYStart += (g_uiCtx.iMarginX * 2) + ((iTextTotalTall > g_iAvatar) ? iTextTotalTall : g_iAvatar);
		}
	}

	g_uiCtx.dPanel.x = iRightXPos;
	g_uiCtx.dPanel.y = iRightSideYStart;
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide - iRightXPos + (g_uiCtx.iMarginX * 2);
	NeoUI::BeginSection();
	{
		if (engine->IsInGame())
		{
			// Show the current server's information
			NeoUI::Label(L"Hostname:", m_wszHostname);
			NeoUI::Label(L"Map:", m_wszMap);
			// TODO: more info, g_PR, scoreboard stuff, etc...
		}
		else
		{
			// Show the news
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			NeoUI::Label(L"News");
			NeoUI::SwapFont(NeoUI::FONT_NTSMALL);

			// Write some headlines
			static constexpr const wchar_t *WSZ_NEWS_HEADLINES[] = {
				L"2024-08-03: NT;RE v7.1 Released",
			};
			for (const wchar_t *wszHeadline : WSZ_NEWS_HEADLINES)
			{
				NeoUI::Label(wszHeadline);
			}
		}
	}
	NeoUI::EndSection();
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopSettings(const MainLoopParam param)
{
	static constexpr void (*P_FN[])(NeoSettings *) = {
		NeoSettings_General,
		NeoSettings_Keys,
		NeoSettings_Mouse,
		NeoSettings_Audio,
		NeoSettings_Video,
	};
	static const wchar_t *WSZ_TABS_LABELS[ARRAYSIZE(P_FN)] = {
		L"Multiplayer", L"Keybinds", L"Mouse", L"Audio", L"Video"
	};

	m_ns.iNextBinding = -1;

	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);

	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, g_pNeoRoot->m_wszDispBtnTexts[MMBTN_OPTIONS], "CtxOptions");
	{
		NeoUI::BeginSection();
		{
			NeoUI::Tabs(WSZ_TABS_LABELS, ARRAYSIZE(WSZ_TABS_LABELS), &m_ns.iCurTab);
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * g_iRowsInScreen;
		NeoUI::BeginSection(true);
		{
			P_FN[m_ns.iCurTab](&m_ns);
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_ns.bBack = true;
				}
				if (NeoUI::Button(L"Legacy").bPressed)
				{
					g_pNeoRoot->GetGameUI()->SendMainMenuCommand("OpenOptionsDialog");
				}
				NeoUI::Pad();
				if (m_ns.bModified)
				{
					if (NeoUI::Button(L"Restore").bPressed)
					{
						NeoSettingsRestore(&m_ns);
					}
					if (NeoUI::Button(L"Accept").bPressed)
					{
						NeoSettingsSave(&m_ns);
					}
				}
			}
			NeoUI::EndHorizontal();
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
	if (!m_ns.bModified && g_uiCtx.bValueEdited)
	{
		m_ns.bModified = true;
	}

	if (m_ns.bBack)
	{
		m_ns.bBack = false;
		m_state = (m_ns.bModified) ? STATE_CONFIRMSETTINGS : STATE_ROOT;
	}
	else if (m_ns.iNextBinding >= 0)
	{
		m_iBindingIdx = m_ns.iNextBinding;
		m_ns.iNextBinding = -1;
		V_swprintf_safe(m_wszBindingText, L"Change binding for: %ls",
						m_ns.keys.vBinds[m_iBindingIdx].wszDisplayText);
		m_state = STATE_KEYCAPTURE;
		engine->StartKeyTrapMode();
	}
}

void CNeoRoot::MainLoopNewGame(const MainLoopParam param)
{
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen + 1);
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, m_wszDispBtnTexts[MMBTN_CREATESERVER], "CtxNewGame");
	{
		NeoUI::BeginSection(true);
		{
			if (NeoUI::Button(L"Map", m_newGame.wszMap).bPressed)
			{
				m_state = STATE_MAPLIST;
			}
			NeoUI::TextEdit(L"Hostname", m_newGame.wszHostname, SZWSZ_LEN(m_newGame.wszHostname));
			NeoUI::SliderInt(L"Max players", &m_newGame.iMaxPlayers, 1, 32);
			NeoUI::TextEdit(L"Password", m_newGame.wszPassword, SZWSZ_LEN(m_newGame.wszPassword));
			NeoUI::RingBoxBool(L"Friendly fire", &m_newGame.bFriendlyFire);
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_ROOT;
				}
				NeoUI::Pad();
				NeoUI::Pad();
				NeoUI::Pad();
				if (NeoUI::Button(L"Start").bPressed)
				{
					if (engine->IsInGame())
					{
						engine->ClientCmd_Unrestricted("disconnect");
					}

					static constexpr int ENTRY_MAX = 64;
					char szMap[ENTRY_MAX + 1] = {};
					char szHostname[ENTRY_MAX + 1] = {};
					char szPassword[ENTRY_MAX + 1] = {};
					g_pVGuiLocalize->ConvertUnicodeToANSI(m_newGame.wszMap, szMap, sizeof(szMap));
					g_pVGuiLocalize->ConvertUnicodeToANSI(m_newGame.wszHostname, szHostname, sizeof(szHostname));
					g_pVGuiLocalize->ConvertUnicodeToANSI(m_newGame.wszPassword, szPassword, sizeof(szPassword));

					ConVarRef("hostname").SetValue(szHostname);
					ConVarRef("sv_password").SetValue(szPassword);
					ConVarRef("mp_friendlyfire").SetValue(m_newGame.bFriendlyFire);

					char cmdStr[256];
					V_sprintf_safe(cmdStr, "maxplayers %d; progress_enable; map \"%s\"", m_newGame.iMaxPlayers, szMap);
					engine->ClientCmd_Unrestricted(cmdStr);

					m_state = STATE_ROOT;
				}
			}
			NeoUI::EndHorizontal();
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopServerBrowser(const MainLoopParam param)
{
	static const wchar_t *GS_NAMES[GS__TOTAL] = {
		L"Internet", L"LAN", L"Friends", L"Fav", L"History", L"Spec"
	};

	bool bEnterServer = false;
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * 2;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, m_wszDispBtnTexts[MMBTN_FINDSERVER], "CtxServerBrowser");
	{
		bool bForceRefresh = false;
		NeoUI::BeginSection();
		{
			const int iPrevTab = m_iServerBrowserTab;
			NeoUI::Tabs(GS_NAMES, ARRAYSIZE(GS_NAMES), &m_iServerBrowserTab);
			if (iPrevTab != m_iServerBrowserTab)
			{
				m_iSelectedServer = -1;
			}
			if (!m_serverBrowser[m_iServerBrowserTab].m_bReloadedAtLeastOnce)
			{
				bForceRefresh = true;
				m_serverBrowser[m_iServerBrowserTab].m_bReloadedAtLeastOnce = true;
			}
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;
			NeoUI::BeginHorizontal(1);
			{
				static constexpr wchar_t *SBLABEL_NAMES[GSIW__TOTAL] = {
					L"Lock", L"VAC", L"Name", L"Map", L"Players", L"Ping",
				};

				for (int i = 0; i < GS__TOTAL; ++i)
				{
					surface()->DrawSetColor((m_sortCtx.col == i) ? COLOR_NEOPANELACCENTBG : COLOR_NEOPANELNORMALBG);
					g_uiCtx.iHorizontalWidth = g_iGSIX[i];
					if (NeoUI::Button(SBLABEL_NAMES[i]).bPressed)
					{
						if (m_sortCtx.col == i)
						{
							m_sortCtx.bDescending = !m_sortCtx.bDescending;
						}
						else
						{
							m_sortCtx.col = static_cast<GameServerInfoW>(i);
						}
						m_bSBFiltModified = true;
					}

					if (param.eMode == NeoUI::MODE_PAINT && m_sortCtx.col == i)
					{
						int iHintTall = g_uiCtx.iMarginY / 3;
						surface()->DrawSetColor(COLOR_NEOPANELTEXTNORMAL);
						if (!m_sortCtx.bDescending)
						{
							NeoUI::GCtxDrawFilledRectXtoX(-g_uiCtx.iHorizontalWidth, 0, 0, iHintTall);
						}
						else
						{
							NeoUI::GCtxDrawFilledRectXtoX(-g_uiCtx.iHorizontalWidth, g_uiCtx.iRowTall - iHintTall, 0, g_uiCtx.iRowTall);
						}
						surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
					}
				}
			}

			// TODO: Should give proper controls over colors through NeoUI
			surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
			surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
			NeoUI::EndHorizontal();
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
		}
		NeoUI::EndSection();
		static constexpr int FILTER_ROWS = 4;
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen - 1);
		if (m_bShowFilterPanel) g_uiCtx.dPanel.tall -= g_uiCtx.iRowTall * FILTER_ROWS;
		NeoUI::BeginSection(true);
		{
			if (m_serverBrowser[m_iServerBrowserTab].m_filteredServers.IsEmpty())
			{
				wchar_t wszInfo[128];
				if (m_serverBrowser[m_iServerBrowserTab].m_bSearching)
				{
					V_swprintf_safe(wszInfo, L"Searching %ls queries...", GS_NAMES[m_iServerBrowserTab]);
				}
				else
				{
					V_swprintf_safe(wszInfo, L"No %ls queries found. Press Refresh to re-check", GS_NAMES[m_iServerBrowserTab]);
				}
				g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
				NeoUI::Label(wszInfo);
				g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
			}
			else
			{
				// NEO TODO (nullsystem): This is fine since there isn't going to be much servers, it'll
				// handle even couple of 100s ok. However if the amount is a concern, NeoUI can be
				// expanded to give Y-offset in the form of filtered array range so it only loops
				// through what's needed instead.
				const auto *sbTab = &m_serverBrowser[m_iServerBrowserTab];
				for (int i = 0; i < sbTab->m_filteredServers.Size(); ++i)
				{
					const auto &server = sbTab->m_filteredServers[i];
					bool bSkipServer = false;
					if (m_sbFilters.bServerNotFull && server.m_nPlayers == server.m_nMaxPlayers) bSkipServer = true;
					else if (m_sbFilters.bHasUsersPlaying && server.m_nPlayers == 0) bSkipServer = true;
					else if (m_sbFilters.bIsNotPasswordProtected && server.m_bPassword) bSkipServer = true;
					else if (m_sbFilters.iAntiCheat == ANTICHEAT_OFF && server.m_bSecure) bSkipServer = true;
					else if (m_sbFilters.iAntiCheat == ANTICHEAT_ON && !server.m_bSecure) bSkipServer = true;
					if (bSkipServer)
					{
						continue;
					}

					const auto btn = NeoUI::Button(L"");
					if (btn.bPressed) // Dummy button, draw over it in paint
					{
						m_iSelectedServer = i;
						if (btn.bKeyPressed || btn.bMouseDoublePressed)
						{
							bEnterServer = true;
						}
					}

					if (param.eMode == NeoUI::MODE_PAINT)
					{
						Color drawColor = COLOR_NEOPANELNORMALBG;
						if (m_iSelectedServer == i) drawColor = COLOR_NEOPANELACCENTBG;
						if (btn.bMouseHover) drawColor = COLOR_NEOPANELSELECTBG;

						surface()->DrawSetColor(drawColor);
						NeoUI::GCtxDrawFilledRectXtoX(0, -g_uiCtx.iRowTall, g_uiCtx.dPanel.wide, 0);

						int xPos = 0;
						const int yPos = g_uiCtx.iYOffset[g_uiCtx.iSection] - g_uiCtx.iRowTall;
						const int fontStartYPos = g_uiCtx.fonts[g_uiCtx.eFont].iYOffset;

						if (server.m_bPassword)
						{
							NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
							surface()->DrawPrintText(L"P", 1);
						}
						xPos += g_iGSIX[GSIW_LOCKED];

						if (server.m_bSecure)
						{
							NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
							surface()->DrawPrintText(L"S", 1);
						}
						xPos += g_iGSIX[GSIW_VAC];

						{
							wchar_t wszServerName[k_cbMaxGameServerName];
							const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(server.GetName(), wszServerName, sizeof(wszServerName));
							NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
							surface()->DrawPrintText(wszServerName, iSize);
						}
						xPos += g_iGSIX[GSIW_NAME];

						{
							// In lower resolution, it may overlap from name, so paint a background here
							NeoUI::GCtxDrawFilledRectXtoX(xPos, -g_uiCtx.iRowTall, g_uiCtx.dPanel.wide, 0);

							wchar_t wszMapName[k_cbMaxGameServerMapName];
							const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(server.m_szMap, wszMapName, sizeof(wszMapName));
							NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
							surface()->DrawPrintText(wszMapName, iSize);
						}
						xPos += g_iGSIX[GSIW_MAP];

						{
							// In lower resolution, it may overlap from name, so paint a background here
							NeoUI::GCtxDrawFilledRectXtoX(xPos, -g_uiCtx.iRowTall, g_uiCtx.dPanel.wide, 0);

							wchar_t wszPlayers[10];
							const int iSize = V_swprintf_safe(wszPlayers, L"%d/%d", server.m_nPlayers, server.m_nMaxPlayers);
							NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
							surface()->DrawPrintText(wszPlayers, iSize);
						}
						xPos += g_iGSIX[GSIW_PLAYERS];

						{
							wchar_t wszPing[10];
							const int iSize = V_swprintf_safe(wszPing, L"%d", server.m_nPing);
							NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
							surface()->DrawPrintText(wszPing, iSize);
						}
					}
				}
			}
		}
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		if (m_bShowFilterPanel) g_uiCtx.dPanel.tall += g_uiCtx.iRowTall * FILTER_ROWS;
		NeoUI::BeginSection();
		{
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 6);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_ROOT;
				}
				if (NeoUI::Button(L"Legacy").bPressed)
				{
					GetGameUI()->SendMainMenuCommand("OpenServerBrowser");
				}
				if (NeoUI::Button(m_bShowFilterPanel ? L"Hide Filters" : L"Show Filters").bPressed)
				{
					m_bShowFilterPanel = !m_bShowFilterPanel;
				}
				if (m_iSelectedServer >= 0)
				{
					if (NeoUI::Button(L"Details").bPressed)
					{
						m_state = STATE_SERVERDETAILS;
						const auto *gameServer = &m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
						m_serverPlayers.RequestList(gameServer->m_NetAdr.GetIP(), gameServer->m_NetAdr.GetQueryPort());
					}
				}
				else
				{
					NeoUI::Pad();
				}
				if (NeoUI::Button(L"Refresh").bPressed || bForceRefresh)
				{
					m_iSelectedServer = -1;
					ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
					CNeoServerList *pServerBrowser = &m_serverBrowser[m_iServerBrowserTab];
					pServerBrowser->m_servers.RemoveAll();
					pServerBrowser->m_filteredServers.RemoveAll();
					if (pServerBrowser->m_hdlRequest)
					{
						steamMM->CancelQuery(pServerBrowser->m_hdlRequest);
						steamMM->ReleaseRequest(pServerBrowser->m_hdlRequest);
						pServerBrowser->m_hdlRequest = nullptr;
					}
					pServerBrowser->RequestList();
				}
				if (m_iSelectedServer >= 0)
				{
					if (bEnterServer || NeoUI::Button(L"Enter").bPressed)
					{
						if (engine->IsInGame())
						{
							engine->ClientCmd_Unrestricted("disconnect");
						}

						ConVarRef("password").SetValue("");
						V_memset(m_wszServerPassword, 0, sizeof(m_wszServerPassword));

						// NEO NOTE (nullsystem): Deal with password protected server
						const auto gameServer = m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
						if (gameServer.m_bPassword)
						{
							m_state = STATE_SERVERPASSWORD;
						}
						else
						{
							char connectCmd[256];
							const char *szAddress = gameServer.m_NetAdr.GetConnectionAddressString();
							V_sprintf_safe(connectCmd, "progress_enable; wait; connect %s", szAddress);
							engine->ClientCmd_Unrestricted(connectCmd);

							m_state = STATE_ROOT;
						}
					}
				}
			}
			NeoUI::EndHorizontal();
			if (m_bShowFilterPanel)
			{
				NeoUI::RingBoxBool(L"Server not full", &m_sbFilters.bServerNotFull);
				NeoUI::RingBoxBool(L"Has users playing", &m_sbFilters.bHasUsersPlaying);
				NeoUI::RingBoxBool(L"Is not password protected", &m_sbFilters.bIsNotPasswordProtected);
				NeoUI::RingBox(L"Anti-cheat", ANTICHEAT_LABELS, ARRAYSIZE(ANTICHEAT_LABELS), &m_sbFilters.iAntiCheat);
			}
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();

}

void CNeoRoot::MainLoopMapList(const MainLoopParam param)
{
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen + 1);
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Pick map", "CtxMapPicker");
	{
		NeoUI::BeginSection(true);
		{
			for (auto &wszMap : m_vWszMaps)
			{
				if (NeoUI::Button(wszMap.wszName).bPressed)
				{
					V_wcscpy_safe(m_newGame.wszMap, wszMap.wszName);
					m_state = STATE_NEWGAME;
				}
			}
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_NEWGAME;
				}
			}
			NeoUI::EndHorizontal();
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopServerDetails(const MainLoopParam param)
{
	const auto *gameServer = &m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * 6;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Server details", "CtxServerDetail");
	{
		NeoUI::BeginSection(true);
		{
			const bool bP = param.eMode == NeoUI::MODE_PAINT;
			wchar_t wszText[128];
			if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->GetName(), wszText, sizeof(wszText));
			NeoUI::Label(L"Name:", wszText);
			if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->m_NetAdr.GetConnectionAddressString(), wszText, sizeof(wszText));
			NeoUI::Label(L"Address:", wszText);
			if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->m_szMap, wszText, sizeof(wszText));
			NeoUI::Label(L"Map:", wszText);
			if (bP) V_swprintf_safe(wszText, L"%d/%d", gameServer->m_nPlayers, gameServer->m_nMaxPlayers);
			NeoUI::Label(L"Ping:", wszText);
			if (bP) V_swprintf_safe(wszText, L"%ls", gameServer->m_bSecure ? L"Enabled" : L"Disabled");
			NeoUI::Label(L"VAC:", wszText);
			if (bP) V_swprintf_safe(wszText, L"%d", gameServer->m_nPing);
			NeoUI::Label(L"Ping:", wszText);

			// TODO: Header same-style as serverlist's header
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = (iTallTotal - g_uiCtx.iRowTall) - g_uiCtx.dPanel.tall;
		NeoUI::BeginSection();
		{
			if (m_serverPlayers.m_players.IsEmpty())
			{
				g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
				NeoUI::Label(L"There are no players in the server.");
				g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
			}
			else
			{
				for (const auto &player : m_serverPlayers.m_players)
				{
					NeoUI::Label(player.wszName);
					// TODO
				}
			}
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_SERVERBROWSER;
				}
			}
			NeoUI::EndHorizontal();
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopPlayerList(const MainLoopParam param)
{
	if (engine->IsInGame())
	{
		const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
		g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
		g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
		g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen + 1);
		g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
		NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Player list", "CtxPlayerList");
		{
			NeoUI::BeginSection(true);
			{
				g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					if (!g_PR->IsConnected(i) || g_PR->IsHLTV(i) || g_PR->IsFakePlayer(i))
					{
						continue;
					}

					const bool bOwnLocalPlayer = g_PR->IsLocalPlayer(i);
					const bool bPlayerMuted = GetClientVoiceMgr()->IsPlayerBlocked(i);
					const char *szPlayerName = g_PR->GetPlayerName(i);

					wchar_t wszInfo[256];
					V_swprintf_safe(wszInfo, L"%ls%s", bOwnLocalPlayer ? L"[LOCAL] " : bPlayerMuted ? L"[MUTED] " : L"[VOICE] ", szPlayerName);
					if (NeoUI::Button(wszInfo).bPressed)
					{
						if (!bOwnLocalPlayer) GetClientVoiceMgr()->SetPlayerBlockedState(i, !bPlayerMuted);
					}
				}
				g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			}
			NeoUI::EndSection();
			g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
			g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
			NeoUI::BeginSection();
			{
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
				{
					if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_ROOT;
					}
				}
				NeoUI::EndHorizontal();
			}
			NeoUI::EndSection();
		}
		NeoUI::EndContext();
	}
	else
	{
		m_state = STATE_ROOT;
	}
}

void CNeoRoot::MainLoopPopup(const MainLoopParam param)
{
	surface()->DrawSetColor(COLOR_NEOPANELPOPUPBG);
	surface()->DrawFilledRect(0, 0, param.wide, param.tall);
	const int tallSplit = param.tall / 3;
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	surface()->DrawFilledRect(0, tallSplit, param.wide, param.tall - tallSplit);

	g_uiCtx.dPanel.wide = g_iRootSubPanelWide * 0.75f;
	g_uiCtx.dPanel.tall = tallSplit;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_uiCtx.dPanel.wide / 2);
	g_uiCtx.dPanel.y = tallSplit + (tallSplit / 2) - g_uiCtx.iRowTall;
	g_uiCtx.bgColor = COLOR_TRANSPARENT;
	if (m_state == STATE_SERVERPASSWORD)
	{
		g_uiCtx.dPanel.y -= g_uiCtx.iRowTall;
	}
	// Technically can get away with have a common context name, since these dialogs
	// don't do anything special with it
	NeoUI::BeginContext(&g_uiCtx, param.eMode, nullptr, "CtxCommonPopupDlg");
	{
		NeoUI::BeginSection(true);
		{
			g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
			switch (m_state)
			{
			case STATE_KEYCAPTURE:
			{
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::Label(m_wszBindingText);
				NeoUI::SwapFont(NeoUI::FONT_NTSMALL);
				NeoUI::Label(L"Press ESC to cancel or DEL to remove keybind");
			}
			break;
			case STATE_CONFIRMSETTINGS:
			{
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::Label(L"Settings changed: Do you want to apply the settings?");
				NeoUI::SwapFont(NeoUI::FONT_NTSMALL);
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 3);
				{
					if (NeoUI::Button(L"Save (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						NeoSettingsSave(&m_ns);
						m_state = STATE_ROOT;
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"Discard (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_ROOT;
					}
				}
				NeoUI::EndHorizontal();
			}
			break;
			case STATE_QUIT:
			{
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::Label(L"Do you want to quit the game?");
				NeoUI::SwapFont(NeoUI::FONT_NTSMALL);
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 3);
				{
					if (NeoUI::Button(L"Quit (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						engine->ClientCmd_Unrestricted("quit");
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"Cancel (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_ROOT;
					}
				}
				NeoUI::EndHorizontal();
			}
			break;
			case STATE_SERVERPASSWORD:
			{
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::Label(L"Enter the server password");
				NeoUI::SwapFont(NeoUI::FONT_NTSMALL);
				g_uiCtx.bTextEditIsPassword = true;
				NeoUI::TextEdit(L"Password:", m_wszServerPassword, SZWSZ_LEN(m_wszServerPassword));
				g_uiCtx.bTextEditIsPassword = false;
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 3);
				{
					if (NeoUI::Button(L"Enter (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						char szServerPassword[ARRAYSIZE(m_wszServerPassword)];
						g_pVGuiLocalize->ConvertUnicodeToANSI(m_wszServerPassword, szServerPassword, sizeof(szServerPassword));
						ConVarRef("password").SetValue(szServerPassword);
						V_memset(m_wszServerPassword, 0, sizeof(m_wszServerPassword));

						const auto gameServer = m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
						char connectCmd[256];
						const char *szAddress = gameServer.m_NetAdr.GetConnectionAddressString();
						V_sprintf_safe(connectCmd, "progress_enable; wait; connect %s", szAddress);
						engine->ClientCmd_Unrestricted(connectCmd);

						m_state = STATE_ROOT;
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"Cancel (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						V_memset(m_wszServerPassword, 0, sizeof(m_wszServerPassword));
						m_state = STATE_SERVERBROWSER;
					}
				}
				NeoUI::EndHorizontal();
			}
			break;
			default:
				break;
			}
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

// NEO NOTE (nullsystem): NeoRootCaptureESC is so that ESC keybinds can be recognized by non-root states, but root
// state still want to have ESC handled by the game as IsVisible/HasFocus isn't reliable indicator to depend on.
// This goes along with NeoToggleconsole which if the toggleconsole is activated on non-root state that can end up
// blocking ESC key from being usable in-game, so have to force it back to root state first to help with
// NeoRootCaptureESC condition. So this will do.

bool NeoRootCaptureESC()
{
	return (g_pNeoRoot && g_pNeoRoot->IsEnabled() && g_pNeoRoot->m_state != STATE_ROOT);
}

void NeoToggleconsole()
{
	if (engine->IsInGame() && g_pNeoRoot)
	{
		g_pNeoRoot->m_state = STATE_ROOT;
	}
	engine->ClientCmd_Unrestricted("toggleconsole");
}
