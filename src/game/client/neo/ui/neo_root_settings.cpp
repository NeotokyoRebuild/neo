#include "neo_root_settings.h"

#include "cdll_client_int.h"
#include "filesystem.h"
#include <IGameUIFuncs.h>
#include <tier3.h>
#include "vgui/ILocalize.h"
#include "inputsystem/iinputsystem.h"
#include "characterset.h"
#include "materialsystem/materialsystem_config.h"
#include <ivoicetweak.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <steam/steam_api.h>
#include "vgui/ISystem.h"
#include "neo_hud_killer_damage_info.h"

#include "neo_ui.h"
#include "neo_root.h"
#include "neo/ui/neo_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CNeoRoot *g_pNeoRoot;
extern NeoUI::Context g_uiCtx;
extern int g_iRowsInScreen;
extern bool g_bOBSDetected;
extern ConVar cl_neo_streamermode;

const wchar_t *QUALITY_LABELS[] = {
	L"Low",
	L"Medium",
	L"High",
	L"Very High",
};

const wchar_t* QUALITY3_LABELS[] = {
	L"Low",
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

enum Quality3Enum
{
	QUALITY3_LOW = 0,
	QUALITY3_HIGH,
	QUALITY3_VERYHIGH,
};

enum ESpeaker
{
	SPEAKER_AUTO = -1,
	SPEAKER_HEADPHONES = 0,
	SPEAKER_STEREO = 2,
	SPEAKER_QUAD = 4,
	SPEAKER_SURROUND_5_1 = 5,
	SPEAKER_SURROUND_7_1 = 7,
};

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

enum WindowModeEnum
{
	WINDOWMODE_FULLSCREEN = 0,
	WINDOWMODE_WINDOWED,
	WINDOWMODE_WINDOWEDNOBORDER,

	WINDOWMODE__TOTAL,
};

static const wchar_t *SPEAKER_CFG_LABELS[] = {
	L"Auto",
	L"Headphones",
	L"2 Speakers (Stereo)",
#ifndef LINUX
	L"4 Speakers (Quadraphonic)",
	L"5.1 Surround",
	L"7.1 Surround",
#endif
};

static const char *DLFILTER_STRMAP[] = {
	"all", "nosounds", "mapsonly", "none"
};

static const wchar_t* AUTOMATIC_LEAN_LABELS[] = {
	L"Disabled",
	L"When aiming",
	L"Always",
};

static inline CUtlVector<ConVarRefEx *> g_vecConVarRefPtrs;

ConVarRefEx::ConVarRefEx(const char *pName, const bool bExcludeGlobalPtrs)
	: ConVarRef(pName)
{
	if (!bExcludeGlobalPtrs)
	{
		g_vecConVarRefPtrs.AddToTail(this);
	}
}

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
	characterset_t breakSet = {};
	breakSet.set[0] = '"';
	NeoSettings::Keys *keys = &ns->keys;
	CUtlBuffer bufAct(0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	if (filesystem->ReadFile("scripts/kb_act.lst", nullptr, bufAct))
	{
		keys->iBindsSize = 0;

		// It's quite specific to the root overridden menu so do it here instead of file
		auto *conbind = &keys->vBinds[keys->iBindsSize++];
		V_strcpy_safe(conbind->szBindingCmd, "neo_toggleconsole");
		V_wcscpy_safe(conbind->wszDisplayText, L"Developer console bind");

		while (bufAct.IsValid() && keys->iBindsSize < ARRAYSIZE(keys->vBinds))
		{
			char szBindingCmd[64];
			char szRawDispText[64];

			bool bIsOk = false;
			bIsOk = bufAct.ParseToken(&breakSet, szBindingCmd, sizeof(szBindingCmd));
			if (!bIsOk) break;
			bIsOk = bufAct.ParseToken(&breakSet, szRawDispText, sizeof(szRawDispText));
			if (!bIsOk) break;

			if (szBindingCmd[0] == '\0') continue;

			wchar_t wszDispText[64];
			if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(szRawDispText))
			{
				V_wcscpy_safe(wszDispText, localizedWszStr);
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(szRawDispText, wszDispText, sizeof(wszDispText));
			}

			const bool bIsBlank = V_strcmp(szBindingCmd, "blank") == 0;
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
				V_strcpy_safe(bind->szBindingCmd, szBindingCmd);
				V_wcscpy_safe(bind->wszDisplayText, wszDispText);
				bind->bcDefault = BUTTON_CODE_NONE;
			}
		}
		AssertMsg(keys->iBindsSize < ARRAYSIZE(keys->vBinds), "Bump the size of the vBinds array");
	}

	CUtlBuffer bufDef(0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	if (keys->iBindsSize > 0 && filesystem->ReadFile("scripts/kb_def.lst", nullptr, bufDef))
	{
		while (bufDef.IsValid())
		{
			char szDefBind[64];
			char szBindingCmd[64];

			bool bIsOk = false;
			bIsOk = bufDef.ParseToken(&breakSet, szDefBind, sizeof(szDefBind));
			if (!bIsOk) break;
			bIsOk = bufDef.ParseToken(&breakSet, szBindingCmd, sizeof(szBindingCmd));
			if (!bIsOk) break;

			for (int i = 0; i < keys->iBindsSize; ++i)
			{
				if (V_strcmp(keys->vBinds[i].szBindingCmd, szBindingCmd) == 0)
				{
					keys->vBinds[i].bcDefault = g_pInputSystem->StringToButtonCode(szDefBind);
					break;
				}
			}
		}
	}

	// Setup crosshairs
	for (int i = 0; i < CROSSHAIR_STYLE__TOTAL; ++i)
	{
		if (CROSSHAIR_FILES[i][0])
		{
			NeoSettings::Crosshair::Texture *pTex = &ns->crosshair.arTextures[i];
			pTex->iTexId = vgui::surface()->CreateNewTextureID();
			vgui::surface()->DrawSetTextureFile(pTex->iTexId, CROSSHAIR_FILES[i], false, false);
			vgui::surface()->DrawGetTextureSize(pTex->iTexId, pTex->iWide, pTex->iTall);
		}
	}

	// NEO NOTE (Adam) Ensures random background on every launch if random background option selected.
	// If new background is different from previous background, the static image will change during load
	// NEO TODO (Adam) Change the background on client shutdown instead somehow?
	NeoSettingsBackgroundsInit(ns);
}

// sv_unlockedchapters is not a consistent way of guaranteeing what background shows up as the menu background.
// Instead, we read all available backgrounds from a second file neo_backgrounds.txt, and overwrite chapterbackgrounds.txt
// to only include the one background that the user is interested in.
void NeoSettingsBackgroundsInit(NeoSettings* ns)
{
#define NEO_BACKGROUNDS_FILENAME "scripts/neo_backgrounds.txt"
#define NEO_RANDOM_BACKGROUND_NAME "Random"
#define NEO_FALLBACK_BACKGROUND_DISPLAYNAME "No backgrounds"
#define NEO_FALLBACK_BACKGROUND_FILENAME "background01"

	if (ns->backgrounds)
	{
		ns->backgrounds->deleteThis();
	}
	ns->backgrounds = new KeyValues( "neo_backgrounds" );
	ns->iCBListSize = 0;

	constexpr auto allocate = [](NeoSettings *ns, int size) {
		ns->p2WszCBList = (wchar_t **)calloc(sizeof(wchar_t *), ns->iCBListSize);
		ns->p2WszCBList[0] = (wchar_t *)calloc(sizeof(wchar_t) * size, ns->iCBListSize);
	};

	// Setup Background Map options
	int dispSize = Max(sizeof(NEO_FALLBACK_BACKGROUND_DISPLAYNAME), sizeof(NEO_RANDOM_BACKGROUND_NAME) + 1);
	if ( !ns->backgrounds->LoadFromFile( g_pFullFileSystem, NEO_BACKGROUNDS_FILENAME, "MOD" ) )
	{ // File empty or unable to load, set to static and return early
		Warning( "Unable to load '%s'\n", NEO_BACKGROUNDS_FILENAME );
		ns->iCBListSize = 1;
		allocate(ns, dispSize);
		g_pVGuiLocalize->ConvertANSIToUnicode(NEO_FALLBACK_BACKGROUND_DISPLAYNAME, ns->p2WszCBList[0], sizeof(wchar_t) * dispSize);
		NeoSettingsBackgroundWrite(ns, NEO_FALLBACK_BACKGROUND_FILENAME);
		return;
	}

	for ( KeyValues* background = ns->backgrounds->GetFirstSubKey(); background != NULL; /*background = background->GetNextKey()*/)
	{ // Iterate once to get the number of options and longest background map name
		const char* displayName = background->GetName();
		if (FStrEq(displayName, "")) // NEO NOTE (Adam) If name missing read will fail
		{ // no display name, skip
			KeyValues* thisKey = background;
			background = background->GetNextKey();
			ns->backgrounds->RemoveSubKey(thisKey);
			continue;
		}

		const char* fileName = background->GetString("fileName");
		if (FStrEq(fileName, ""))
		{ // no file name, skip
			KeyValues* thisKey = background;
			background = background->GetNextKey();
			ns->backgrounds->RemoveSubKey(thisKey);
			continue;
		}

		ns->iCBListSize++;
		dispSize = Max(dispSize, (V_strlen(displayName) + 1));
		background = background->GetNextKey();
	}
	const int wDispSize = sizeof(wchar_t) * dispSize;
	
	// Random Background Option
	ns->iCBListSize++;
	allocate(ns, dispSize);
	KeyValues* background = ns->backgrounds->GetFirstSubKey();
	// iterate through background maps and set their names
	for (int i = 0, offset = 0; i < ns->iCBListSize - 1; ++i, offset += dispSize)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(background->GetName(), ns->p2WszCBList[0] + offset, wDispSize);
		ns->p2WszCBList[i] = ns->p2WszCBList[0] + offset;
		background = background->GetNextKey();
	}

	// Set last option to random
	const int offset = (ns->iCBListSize - 1) * dispSize;
	g_pVGuiLocalize->ConvertANSIToUnicode(ns->iCBListSize == 1 ? NEO_FALLBACK_BACKGROUND_DISPLAYNAME : NEO_RANDOM_BACKGROUND_NAME, ns->p2WszCBList[0] + offset, wDispSize);
	ns->p2WszCBList[ns->iCBListSize - 1] = ns->p2WszCBList[0] + offset;

	// write selected background name
	NeoSettingsBackgroundWrite(ns);
}

void NeoSettingsBackgroundWrite(const NeoSettings* ns, const char* backgroundName)
{
	// Select background name
	if (!backgroundName)
	{
		const int selectedBackground = ns->cvr.sv_unlockedchapters.GetInt() + 1;
		int i = 1;
		const int iFinal = selectedBackground >= ns->iCBListSize ? RandomInt(1, ns->iCBListSize - 1) : selectedBackground;
		KeyValues* background;
		for (background = ns->backgrounds->GetFirstSubKey(); background != NULL && i < iFinal; (background = background->GetNextKey()) && i++)
		{ // Skip to the desired background
		}
		backgroundName = background ? background->GetString("fileName") : NEO_FALLBACK_BACKGROUND_DISPLAYNAME;
	}

	// Overwrite CHAPTER_BACKGROUNDS_FILENAME with selected background
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	bpr(0, buf, "// Auto Generated on game launch. Set background in game through the options menu from those available in scripts/neo_backgrounds.txt\n");
	bpr(0, buf, "\"chapters\"\n");
	bpr(0, buf, "{\n" );
	bpr(1, buf, "1 \"");
	bpr(0, buf, backgroundName);
	bpr(0, buf, "\"\n");
	bpr(0, buf, "}\n");

#define NEO_CHAPTER_BACKGROUNDS_FILENAME "scripts/chapterbackgrounds.txt"
	FileHandle_t fh = g_pFullFileSystem->Open( NEO_CHAPTER_BACKGROUNDS_FILENAME, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close( fh );
	}
	else
	{
		Warning( "Unable to open '%s' for writing\n", NEO_CHAPTER_BACKGROUNDS_FILENAME );
	}
}

void NeoSettingsDeinit(NeoSettings *ns)
{
	free(ns->video.p2WszVmDispList[0]);
	free(ns->video.p2WszVmDispList);

	free(ns->p2WszCBList[0]);
	free(ns->p2WszCBList);
	if (ns->backgrounds)
	{
		ns->backgrounds->deleteThis();
	}
}

void NeoSettingsRestore(NeoSettings *ns, const NeoSettings::Keys::Flags flagsKeys)
{
	ns->bModified = false;
	ns->bIsValid = true;
	NeoSettings::CVR *cvr = &ns->cvr;
	{
		NeoSettings::General *pGeneral = &ns->general;
		g_pVGuiLocalize->ConvertANSIToUnicode(cvr->neo_name.GetString(), pGeneral->wszNeoName, sizeof(pGeneral->wszNeoName));
		g_pVGuiLocalize->ConvertANSIToUnicode(cvr->neo_clantag.GetString(), pGeneral->wszNeoClantag, sizeof(pGeneral->wszNeoClantag));
		pGeneral->bOnlySteamNick = cvr->cl_onlysteamnick.GetBool();
		pGeneral->bMarkerSpecOnlyClantag = cvr->cl_neo_clantag_friendly_marker_spec_only.GetBool();
		pGeneral->iFov = cvr->neo_fov.GetInt();
		pGeneral->iViewmodelFov = cvr->neo_viewmodel_fov_offset.GetInt();
		pGeneral->bAimHold = cvr->neo_aim_hold.GetBool();
		pGeneral->bReloadEmpty = cvr->cl_autoreload_when_empty.GetBool();
		pGeneral->bViewmodelRighthand = cvr->cl_righthand.GetBool();
		pGeneral->bLeanViewmodelOnly = cvr->cl_neo_lean_viewmodel_only.GetBool();
		pGeneral->iLeanAutomatic = cvr->cl_neo_lean_automatic.GetInt();
		pGeneral->bHipFireCrosshair = cvr->cl_neo_crosshair_hip_fire.GetBool();
		pGeneral->bShowSquadList = cvr->cl_neo_squad_hud_original.GetBool();
		pGeneral->bShowPlayerSprays = !(cvr->cl_spraydisable.GetBool()); // Inverse
		pGeneral->bShowHints = cvr->cl_neo_showhints.GetBool();
		pGeneral->bShowPos = cvr->cl_showpos.GetBool();
		pGeneral->iShowFps = cvr->cl_showfps.GetInt();
		{
			const char *szDlFilter = cvr->cl_downloadfilter.GetString();
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
		pGeneral->bStreamerMode = cvr->cl_neo_streamermode.GetBool();
		pGeneral->bAutoDetectOBS = cvr->cl_neo_streamermode_autodetect_obs.GetBool();
		pGeneral->bEnableRangeFinder = cvr->cl_neo_hud_rangefinder_enabled.GetBool();
		pGeneral->bExtendedKillfeed = cvr->cl_neo_hud_extended_killfeed.GetBool();
		pGeneral->iBackground = clamp(cvr->sv_unlockedchapters.GetInt(), 0, ns->iCBListSize - 1);
		pGeneral->iKdinfoToggletype = cvr->cl_neo_kdinfo_toggletype.GetInt();
		NeoSettingsBackgroundWrite(ns);
		NeoUI::ResetTextures();
	}
	{
		NeoSettings::Keys *pKeys = &ns->keys;
		pKeys->bWeaponFastSwitch = cvr->hud_fastswitch.GetBool();
		pKeys->bDeveloperConsole = cvr->cl_neo_toggleconsole.GetBool();
		if (!(flagsKeys & NeoSettings::Keys::SKIP_KEYS))
		{
			for (int i = 0; i < pKeys->iBindsSize; ++i)
			{
				auto *bind = &pKeys->vBinds[i];
				bind->bcNext = bind->bcCurrent = gameuifuncs->GetButtonCodeForBind(bind->szBindingCmd);
			}
		}
	}
	{
		NeoSettings::Mouse *pMouse = &ns->mouse;
		pMouse->flSensitivity = cvr->sensitivity.GetFloat();
		pMouse->flZoomSensitivityRatio = cvr->zoom_sensitivity_ratio.GetFloat();
		pMouse->bRawInput = cvr->m_rawinput.GetBool();
		pMouse->bFilter = cvr->m_filter.GetBool();
		pMouse->bReverse = (cvr->m_pitch.GetFloat() < 0.0f);
		pMouse->bCustomAccel = (cvr->m_customaccel.GetInt() == 3);
		pMouse->flExponent = cvr->m_customaccel_exponent.GetFloat();
	}
	{
		NeoSettings::Audio *pAudio = &ns->audio;

		// Output
		pAudio->flVolMain = cvr->volume.GetFloat();
		pAudio->flVolMusic = cvr->snd_musicvolume.GetFloat();
		pAudio->flVolVictory = cvr->snd_victory_volume.GetFloat();
		pAudio->flVolPing = cvr->snd_ping_volume.GetFloat();
		pAudio->iSoundSetup = 0;
		switch (cvr->snd_surround_speakers.GetInt())
		{
		break; case SPEAKER_HEADPHONES:		pAudio->iSoundSetup = 1;
		break; case SPEAKER_STEREO:			pAudio->iSoundSetup = 2;
#ifndef LINUX
		break; case SPEAKER_QUAD:			pAudio->iSoundSetup = 3;
		break; case SPEAKER_SURROUND_5_1:	pAudio->iSoundSetup = 4;
		break; case SPEAKER_SURROUND_7_1:	pAudio->iSoundSetup = 5;
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
		pAudio->bMicBoost = (engine->GetVoiceTweakAPI()->GetControlFloat(MicBoost) > 0.0f);
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

		const bool bIsWindowed = g_pMaterialSystem->GetCurrentConfigForVideoCard().Windowed();
		pVideo->iWindow = static_cast<int>(bIsWindowed);
		if (bIsWindowed)
		{
			pVideo->iWindow += static_cast<int>(g_pMaterialSystem->GetCurrentConfigForVideoCard().NoWindowBorder());
		}
		const int queueMode = cvr->mat_queue_mode.GetInt();
		pVideo->iCoreRendering = (queueMode == -1 || queueMode == 2) ? THREAD_MULTI : THREAD_SINGLE;
		pVideo->iModelDetail = 2 - cvr->r_rootlod.GetInt(); // Inverse, highest = 0, lowest = 2
		pVideo->iTextureDetail = 3 - (cvr->mat_picmip.GetInt() + 1); // Inverse+1, highest = -1, lowest = 2
		// Shader detail
		//					mat_reducefillrate			r_lightmap_bicubic
		// Low:						1							0
		// High:					0							0
		// Very High:				0							1
		pVideo->iShaderDetail = (cvr->r_lightmap_bicubic.GetBool()) ?	QUALITY3_VERYHIGH :
								(cvr->mat_reducefillrate.GetBool()) ?	QUALITY3_LOW :
																		QUALITY3_HIGH;
		
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
	{
		NeoSettings::Crosshair *pCrosshair = &ns->crosshair;
		const bool bImported = ImportCrosshair(&pCrosshair->info, cvr->cl_neo_crosshair.GetString());
		if (!bImported)
		{
			ImportCrosshair(&pCrosshair->info, NEO_CROSSHAIR_DEFAULT);
		}
		pCrosshair->bPreviewDynamicAccuracy = false;
		pCrosshair->eClipboardInfo = XHAIREXPORTNOTIFY_NONE;
		pCrosshair->bNetworkCrosshair = cvr->cl_neo_crosshair_network.GetBool();
		pCrosshair->bInaccuracyInScope = cvr->cl_neo_crosshair_scope_inaccuracy.GetBool();
		pCrosshair->bHipFireCrosshair = cvr->cl_neo_crosshair_hip_fire.GetBool();
	}
}

void NeoToggleConsoleEnforce()
{
	// NEO JANK (nullsystem): Try to unbind toggleconsole when possible
	const auto toggleConsoleBind = gameuifuncs->GetButtonCodeForBind("toggleconsole");
	const auto neotoggleConsoleBind = gameuifuncs->GetButtonCodeForBind("neo_toggleconsole");
	const char *bindBtnName = g_pInputSystem->ButtonCodeToString(toggleConsoleBind);
	if (bindBtnName && bindBtnName[0])
	{
		char szCmdStr[128];
		V_sprintf_safe(szCmdStr, "unbind \"%s\"\n", bindBtnName);
		engine->ClientCmd_Unrestricted(szCmdStr);
	}

	// NEO JANK (nullsystem): Try to bind to ` if this is none. This should always be bind
	// wether the console is enabled or not. If they're on a keyboard layout that can't utilise this
	// key, they can just rebind it to another key themselves.
	if (neotoggleConsoleBind <= BUTTON_CODE_NONE)
	{
		engine->ClientCmd_Unrestricted("bind ` neo_toggleconsole");
	}
	else if (toggleConsoleBind == neotoggleConsoleBind)
	{
		// Could be unbinded by the unbind at this point if toggleconsole overlaps, so bring it back to
		// neo_toggleconsole
		const char *bindBtnName = g_pInputSystem->ButtonCodeToString(neotoggleConsoleBind);
		if (bindBtnName && bindBtnName[0])
		{
			char szCmdStr[128];
			V_sprintf_safe(szCmdStr, "bind \"%s\" neo_toggleconsole\n", bindBtnName);
			engine->ClientCmd_Unrestricted(szCmdStr);
		}
	}
}

void NeoSettingsSave(const NeoSettings *ns)
{
	const_cast<NeoSettings *>(ns)->bModified = false;
	auto *cvr = const_cast<NeoSettings::CVR *>(&ns->cvr);
	{
		const NeoSettings::General *pGeneral = &ns->general;
		char neoNameText[sizeof(pGeneral->wszNeoName)];
		g_pVGuiLocalize->ConvertUnicodeToANSI(pGeneral->wszNeoName, neoNameText, sizeof(neoNameText));
		cvr->neo_name.SetValue(neoNameText);
		char neoClantagText[sizeof(pGeneral->wszNeoClantag)];
		g_pVGuiLocalize->ConvertUnicodeToANSI(pGeneral->wszNeoClantag, neoClantagText, sizeof(neoClantagText));
		cvr->neo_clantag.SetValue(neoClantagText);
		cvr->cl_onlysteamnick.SetValue(pGeneral->bOnlySteamNick);
		cvr->cl_neo_clantag_friendly_marker_spec_only.SetValue(pGeneral->bMarkerSpecOnlyClantag);
		cvr->neo_fov.SetValue(pGeneral->iFov);
		cvr->neo_viewmodel_fov_offset.SetValue(pGeneral->iViewmodelFov);
		cvr->neo_aim_hold.SetValue(pGeneral->bAimHold);
		cvr->cl_autoreload_when_empty.SetValue(pGeneral->bReloadEmpty);
		cvr->cl_righthand.SetValue(pGeneral->bViewmodelRighthand);
		cvr->cl_neo_lean_viewmodel_only.SetValue(pGeneral->bLeanViewmodelOnly);
		cvr->cl_neo_lean_automatic.SetValue(pGeneral->iLeanAutomatic);
		cvr->cl_neo_crosshair_hip_fire.SetValue(pGeneral->bHipFireCrosshair);
		cvr->cl_neo_squad_hud_original.SetValue(pGeneral->bShowSquadList);
		cvr->cl_spraydisable.SetValue(!pGeneral->bShowPlayerSprays); // Inverse
		cvr->cl_neo_showhints.SetValue(pGeneral->bShowHints);
		cvr->cl_showpos.SetValue(pGeneral->bShowPos);
		cvr->cl_showfps.SetValue(pGeneral->iShowFps);
		cvr->cl_downloadfilter.SetValue(DLFILTER_STRMAP[pGeneral->iDlFilter]);
		cvr->cl_neo_streamermode.SetValue(pGeneral->bStreamerMode);
		cvr->cl_neo_streamermode_autodetect_obs.SetValue(pGeneral->bAutoDetectOBS);
		cvr->cl_neo_hud_rangefinder_enabled.SetValue(pGeneral->bEnableRangeFinder);
		cvr->cl_neo_hud_extended_killfeed.SetValue(pGeneral->bExtendedKillfeed);
		cvr->sv_unlockedchapters.SetValue(pGeneral->iBackground);
		cvr->cl_neo_kdinfo_toggletype.SetValue(pGeneral->iKdinfoToggletype);
		NeoSettingsBackgroundWrite(ns);
	}
	{
		const NeoSettings::Keys *pKeys = &ns->keys;
		cvr->hud_fastswitch.SetValue(pKeys->bWeaponFastSwitch);
		NeoToggleConsoleEnforce();
		cvr->cl_neo_toggleconsole.SetValue(pKeys->bDeveloperConsole);
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			const auto *bind = &pKeys->vBinds[i];
			if (bind->szBindingCmd[0] != '\0' && bind->bcCurrent > KEY_NONE)
			{
				char cmdStr[128];
				const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcCurrent);
				V_sprintf_safe(cmdStr, "unbind \"%s\"\n", bindBtnName);
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			auto *bind = const_cast<NeoSettings::Keys::Bind *>(&pKeys->vBinds[i]);
			if (bind->szBindingCmd[0] != '\0' && bind->bcNext > KEY_NONE)
			{
				char cmdStr[128];
				const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcNext);
				V_sprintf_safe(cmdStr, "bind \"%s\" \"%s\"\n", bindBtnName, bind->szBindingCmd);
				engine->ClientCmd_Unrestricted(cmdStr);
			}
			bind->bcCurrent = bind->bcNext;
		}
		// Reset the cache to none so it'll refresh on next KeyCodeTyped
		const_cast<NeoSettings::Keys *>(pKeys)->bcConsole = KEY_NONE;
	}
	{
		const NeoSettings::Mouse *pMouse = &ns->mouse;
		cvr->sensitivity.SetValue(pMouse->flSensitivity);
		cvr->zoom_sensitivity_ratio.SetValue(pMouse->flZoomSensitivityRatio);
		cvr->m_rawinput.SetValue(pMouse->bRawInput);
		cvr->m_filter.SetValue(pMouse->bFilter);
		const float absPitch = abs(cvr->m_pitch.GetFloat());
		cvr->m_pitch.SetValue(pMouse->bReverse ? -absPitch : absPitch);
		cvr->m_customaccel.SetValue(pMouse->bCustomAccel ? 3 : 0);
		cvr->m_customaccel_exponent.SetValue(pMouse->flExponent);
	}
	{
		const NeoSettings::Audio *pAudio = &ns->audio;

		static constexpr int SURROUND_RE_MAP[] = {
			SPEAKER_AUTO,
			SPEAKER_HEADPHONES,
			SPEAKER_STEREO,
#ifndef LINUX
			SPEAKER_QUAD,
			SPEAKER_SURROUND_5_1,
			SPEAKER_SURROUND_7_1,
#endif
		};

		// Output
		cvr->volume.SetValue(pAudio->flVolMain);
		cvr->snd_musicvolume.SetValue(pAudio->flVolMusic);
		cvr->snd_victory_volume.SetValue(pAudio->flVolVictory);
		cvr->snd_ping_volume.SetValue(pAudio->flVolPing);
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

			MaterialSystem_Config_t newMaterialSystemCfg = g_pMaterialSystem->GetCurrentConfigForVideoCard();
			newMaterialSystemCfg.SetFlag(MATSYS_VIDCFG_FLAGS_NO_WINDOW_BORDER, pVideo->iWindow == WINDOWMODE_WINDOWEDNOBORDER);
			g_pMaterialSystem->OverrideConfig(newMaterialSystemCfg, true);
		}
		cvr->mat_queue_mode.SetValue((pVideo->iCoreRendering == THREAD_MULTI) ? 2 : 0);
		cvr->r_rootlod.SetValue(2 - pVideo->iModelDetail);
		cvr->mat_picmip.SetValue(2 - pVideo->iTextureDetail);
		cvr->mat_reducefillrate.SetValue(pVideo->iShaderDetail == QUALITY3_LOW);
		cvr->r_lightmap_bicubic.SetValue(pVideo->iShaderDetail == QUALITY3_VERYHIGH);
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
	{
		const NeoSettings::Crosshair *pCrosshair = &ns->crosshair;
		char szSequence[NEO_XHAIR_SEQMAX];
		ExportCrosshair(&pCrosshair->info, szSequence);
		cvr->cl_neo_crosshair.SetValue(szSequence);
		cvr->cl_neo_crosshair_network.SetValue(pCrosshair->bNetworkCrosshair);
		cvr->cl_neo_crosshair_scope_inaccuracy.SetValue(pCrosshair->bInaccuracyInScope);
		cvr->cl_neo_crosshair_hip_fire.SetValue(pCrosshair->bHipFireCrosshair);
	}

	engine->ClientCmd_Unrestricted("host_writeconfig");
	engine->ClientCmd_Unrestricted("mat_savechanges");
}

void NeoSettingsResetToDefault(NeoSettings *ns)
{
	// NEO NOTE (nullsystem): The only thing left out is video mode and volume, but that's fine
	// as we shouldn't need to be altering those
	for (ConVarRefEx *convar : g_vecConVarRefPtrs)
	{
		convar->SetValue(convar->GetDefault());
	}

	NeoSettings::Keys *pKeys = &ns->keys;
	for (int i = 0; i < pKeys->iBindsSize; ++i)
	{
		const auto *bind = &pKeys->vBinds[i];
		if (bind->szBindingCmd[0] != '\0' && bind->bcCurrent > KEY_NONE)
		{
			char cmdStr[128];
			const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcCurrent);
			V_sprintf_safe(cmdStr, "unbind \"%s\"\n", bindBtnName);
			engine->ClientCmd_Unrestricted(cmdStr);
		}
	}
	for (int i = 0; i < pKeys->iBindsSize; ++i)
	{
		auto *bind = &pKeys->vBinds[i];
		if (bind->szBindingCmd[0] != '\0' && bind->bcDefault > KEY_NONE)
		{
			char cmdStr[128];
			const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcDefault);
			V_sprintf_safe(cmdStr, "bind \"%s\" \"%s\"\n", bindBtnName, bind->szBindingCmd);
			engine->ClientCmd_Unrestricted(cmdStr);
		}
		bind->bcCurrent = bind->bcNext = bind->bcDefault;
	}

	engine->ClientCmd_Unrestricted("host_writeconfig");

	// NEO JANK (nullsystem): For some reason, bind -> gameuifuncs->GetButtonCodeForBind is too quick for
	// the game engine at this point. So just omit setting binds in restore since we already have anyway.
	NeoSettingsRestore(ns, NeoSettings::Keys::SKIP_KEYS);
}

static const wchar_t *DLFILTER_LABELS[] = {
	L"Allow all custom files from server",
	L"Do not download custom sounds",
	L"Only allow map files",
	L"Do not download any custom files",
};
static const wchar_t *SHOWFPS_LABELS[] = { L"Disabled", L"Enabled (FPS)", L"Enabled (Smooth FPS)", };
static_assert(ARRAYSIZE(DLFILTER_STRMAP) == ARRAYSIZE(DLFILTER_LABELS));

static const wchar_t *KDMGINFO_TOGGLETYPE_LABELS[KDMGINFO_TOGGLETYPE__TOTAL] = {
	L"Always", // KDMGINFO_TOGGLETYPE_ROUND
	L"Reset per match", // KDMGINFO_TOGGLETYPE_MATCH
	L"Never", // KDMGINFO_TOGGLETYPE_NEVER
};

void NeoSettings_General(NeoSettings *ns)
{
	NeoSettings::General *pGeneral = &ns->general;
	NeoUI::TextEdit(L"Name", pGeneral->wszNeoName, MAX_PLAYER_NAME_LENGTH - 1);
	NeoUI::TextEdit(L"Clan tag", pGeneral->wszNeoClantag, NEO_MAX_CLANTAG_LENGTH - 1);
	NeoUI::RingBoxBool(L"Show only steam name", &pGeneral->bOnlySteamNick);
	NeoUI::RingBoxBool(L"Friendly marker spectator only clantags", &pGeneral->bMarkerSpecOnlyClantag);

	wchar_t wszTotalClanAndName[NEO_MAX_DISPLAYNAME];
	ns->bIsValid = GetClNeoDisplayName(wszTotalClanAndName,
			pGeneral->wszNeoName, pGeneral->wszNeoClantag,
			((pGeneral->bOnlySteamNick) ?
			 	CL_NEODISPLAYNAME_FLAG_CHECK | CL_NEODISPLAYNAME_FLAG_ONLYSTEAMNICK :
				CL_NEODISPLAYNAME_FLAG_CHECK));
	if (ns->bIsValid)
	{
		NeoUI::Label(L"Display name", wszTotalClanAndName);
	}
	else
	{
		NeoUI::BeginOverrideFgColor(COLOR_RED);
		NeoUI::Label(L"Display name (invalid)", wszTotalClanAndName);
		NeoUI::EndOverrideFgColor();
	}

	NeoUI::SliderInt(L"FOV", &pGeneral->iFov, 75, 110);
	NeoUI::SliderInt(L"Viewmodel FOV Offset", &pGeneral->iViewmodelFov, -20, 40);
	NeoUI::RingBoxBool(L"Aim hold", &pGeneral->bAimHold);
	NeoUI::RingBoxBool(L"Reload empty", &pGeneral->bReloadEmpty);
	NeoUI::RingBoxBool(L"Right hand viewmodel", &pGeneral->bViewmodelRighthand);
	NeoUI::RingBoxBool(L"Lean viewmodel only", &pGeneral->bLeanViewmodelOnly);
	NeoUI::RingBox(L"Automatic leaning", AUTOMATIC_LEAN_LABELS, ARRAYSIZE(AUTOMATIC_LEAN_LABELS), &pGeneral->iLeanAutomatic);
	NeoUI::RingBoxBool(L"Classic squad list", &pGeneral->bShowSquadList);
	NeoUI::RingBoxBool(L"Show hints", &pGeneral->bShowHints);
	NeoUI::RingBoxBool(L"Show position", &pGeneral->bShowPos);
	NeoUI::RingBox(L"Show FPS", SHOWFPS_LABELS, ARRAYSIZE(SHOWFPS_LABELS), &pGeneral->iShowFps);
	NeoUI::RingBoxBool(L"Show rangefinder", &pGeneral->bEnableRangeFinder);
	NeoUI::RingBoxBool(L"Extended Killfeed", &pGeneral->bExtendedKillfeed);
	NeoUI::RingBox(L"Killer damage info auto show", KDMGINFO_TOGGLETYPE_LABELS, KDMGINFO_TOGGLETYPE__TOTAL, &pGeneral->iKdinfoToggletype);

	
	NeoUI::HeadingLabel(L"MAIN MENU");
	NeoUI::RingBox(L"Selected Background", const_cast<const wchar_t **>(ns->p2WszCBList), ns->iCBListSize, &pGeneral->iBackground);

	NeoUI::HeadingLabel(L"STREAMER MODE");
	NeoUI::RingBoxBool(L"Streamer mode", &pGeneral->bStreamerMode);
	NeoUI::RingBoxBool(L"Auto streamer mode (requires restart)", &pGeneral->bAutoDetectOBS);
	NeoUI::Label(L"OBS detection", g_bOBSDetected ? L"OBS detected on startup" : L"Not detected on startup");

	NeoUI::HeadingLabel(L"DOWNLOADING");
	NeoUI::RingBoxBool(L"Show player spray", &pGeneral->bShowPlayerSprays);
	NeoUI::RingBox(L"Download filter", DLFILTER_LABELS, ARRAYSIZE(DLFILTER_LABELS), &pGeneral->iDlFilter);

	NeoUI::HeadingLabel(L"SPRAY");
	if (IsInGame())
	{
		NeoUI::HeadingLabel(L"Disconnect to update in-game spray");
	}
	else
	{
		NeoUI::SetPerRowLayout(3);
		{
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			if (NeoUI::Button(L"Import").bPressed)
			{
				if (g_pNeoRoot->m_pFileIODialog)
				{
					g_pNeoRoot->m_pFileIODialog->MarkForDeletion();
				}
				g_pNeoRoot->m_pFileIODialog = new vgui::FileOpenDialog(g_pNeoRoot, "Import spray", vgui::FOD_OPEN);
				g_pNeoRoot->m_eFileIOMode = CNeoRoot::FILEIODLGMODE_SPRAY;
				g_pNeoRoot->m_pFileIODialog->AddFilter("*.jpg;*.jpeg;*.png;*.vtf", "Images (JPEG, PNG, VTF)", true);
				g_pNeoRoot->m_pFileIODialog->AddFilter("*.jpg;*.jpeg", "JPEG Image", false);
				g_pNeoRoot->m_pFileIODialog->AddFilter("*.png", "PNG Image", false);
				g_pNeoRoot->m_pFileIODialog->AddFilter("*.vtf", "VTF Image", false);
				g_pNeoRoot->m_pFileIODialog->DoModal();
			}
			if (NeoUI::Button(L"Pick").bPressed)
			{
				g_pNeoRoot->m_state = STATE_SPRAYPICKER;
				g_pNeoRoot->m_bSprayGalleryRefresh = true;
			}
			if (NeoUI::Button(L"Delete").bPressed)
			{
				g_pNeoRoot->m_state = STATE_SPRAYDELETER;
				g_pNeoRoot->m_bSprayGalleryRefresh = true;
			}
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;
		}
	}

	NeoUI::SetPerRowLayout(1, nullptr, g_uiCtx.layout.iRowTall * 6);
	NeoUI::ImageTexture("vgui/logos/ui/spray", L"No spray applied", TEXTURE_GROUP_DECAL);
}

void NeoSettings_Keys(NeoSettings *ns)
{
	NeoSettings::Keys *pKeys = &ns->keys;
	NeoUI::RingBoxBool(L"Weapon fastswitch", &pKeys->bWeaponFastSwitch);
	NeoUI::RingBoxBool(L"Developer console", &pKeys->bDeveloperConsole);
	g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
	for (int i = 0; i < pKeys->iBindsSize; ++i)
	{
		const auto &bind = pKeys->vBinds[i];
		if (bind.szBindingCmd[0] == '\0')
		{
			NeoUI::HeadingLabel(bind.wszDisplayText);
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
	NeoUI::Slider(L"Zoom Sensitivity Ratio", &pMouse->flZoomSensitivityRatio, 0.f, 10.0f, 2, 0.25f);
	NeoUI::RingBoxBool(L"Raw input", &pMouse->bRawInput);
	NeoUI::RingBoxBool(L"Mouse Filter", &pMouse->bFilter);
	NeoUI::RingBoxBool(L"Mouse Reverse", &pMouse->bReverse);
	NeoUI::RingBoxBool(L"Custom Acceleration", &pMouse->bCustomAccel);
	NeoUI::Slider(L"Exponent", &pMouse->flExponent, 1.0f, 1.4f, 2, 0.1f);
}

void NeoSettings_Audio(NeoSettings *ns)
{
	NeoSettings::Audio *pAudio = &ns->audio;
	NeoUI::Slider(L"Main Volume", &pAudio->flVolMain, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Music Volume", &pAudio->flVolMusic, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Victory Volume", &pAudio->flVolVictory, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Ping Volume", &pAudio->flVolPing, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::RingBox(L"Sound Setup", SPEAKER_CFG_LABELS, ARRAYSIZE(SPEAKER_CFG_LABELS), &pAudio->iSoundSetup);
	NeoUI::RingBox(L"Sound Quality", QUALITY_LABELS, 3, &pAudio->iSoundQuality);
	NeoUI::RingBoxBool(L"Mute Audio on un-focus", &pAudio->bMuteAudioUnFocus);
	NeoUI::RingBoxBool(L"Voice Enabled", &pAudio->bVoiceEnabled);
	NeoUI::Slider(L"Voice Receive", &pAudio->flVolVoiceRecv, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::RingBoxBool(L"Microphone Boost", &pAudio->bMicBoost);
	IVoiceTweak_s *pVoiceTweak = engine->GetVoiceTweakAPI();
	const bool bTweaking = pVoiceTweak->IsStillTweaking();
	if (bTweaking && g_uiCtx.eMode == NeoUI::MODE_PAINT)
	{
		// Only fetch the value at interval as immediate is too quick/flickers, and give a longer delay when
		// it goes from sound to no sound.
		static constexpr float FL_FETCH_INTERVAL = 0.1f;
		static constexpr float FL_SILENCE_INTERVAL = 0.4f;
		const float flNowSpeakingVol = pVoiceTweak->GetControlFloat(SpeakingVolume);
		if ((flNowSpeakingVol > 0.0f && pAudio->flLastFetchInterval + FL_FETCH_INTERVAL < gpGlobals->curtime) ||
								(flNowSpeakingVol == 0.0f && pAudio->flSpeakingVol > 0.0f &&
								 pAudio->flLastFetchInterval + FL_SILENCE_INTERVAL < gpGlobals->curtime))
		{
			pAudio->flSpeakingVol = flNowSpeakingVol;
			pAudio->flLastFetchInterval = gpGlobals->curtime;
		}
		vgui::surface()->DrawSetColor(COLOR_NEOPANELMICTEST);
		vgui::surface()->DrawFilledRect(g_uiCtx.rWidgetArea.x0,
										g_uiCtx.rWidgetArea.y0 + g_uiCtx.layout.iRowTall,
										g_uiCtx.rWidgetArea.x0 + (pAudio->flSpeakingVol * g_uiCtx.irWidgetWide),
										g_uiCtx.rWidgetArea.y1 + g_uiCtx.layout.iRowTall);
		vgui::surface()->DrawSetColor(COLOR_TRANSPARENT);
		g_uiCtx.selectBgColor = COLOR_TRANSPARENT;
	}
	if (NeoUI::Button(L"Microphone Tester",
					  bTweaking ? L"Stop testing" : L"Start testing").bPressed)
	{
		bTweaking ? pVoiceTweak->EndVoiceTweakMode() : (void)pVoiceTweak->StartVoiceTweakMode();
		pAudio->flSpeakingVol = 0.0f;
		pAudio->flLastFetchInterval = 0.0f;
	}
	if (bTweaking && g_uiCtx.eMode == NeoUI::MODE_PAINT)
	{
		vgui::surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		g_uiCtx.selectBgColor = COLOR_NEOPANELSELECTBG;
	}
}

static const wchar_t *WINDOW_MODE[WINDOWMODE__TOTAL] = { L"Fullscreen", L"Windowed", L"Windowed (Borderless)" };
static const wchar_t *QUEUE_MODE[] = { L"Single", L"Multi", };
static const wchar_t *QUALITY2_LABELS[] = { L"Low", L"High" };
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
	NeoUI::RingBox(L"Window", WINDOW_MODE, WINDOWMODE__TOTAL, &pVideo->iWindow);
	NeoUI::RingBox(L"Core Rendering", QUEUE_MODE, ARRAYSIZE(QUEUE_MODE), &pVideo->iCoreRendering);
	NeoUI::RingBox(L"Model detail", QUALITY_LABELS, 3, &pVideo->iModelDetail);
	NeoUI::RingBox(L"Texture detail", QUALITY_LABELS, 4, &pVideo->iTextureDetail);
	NeoUI::RingBox(L"Shader detail", QUALITY3_LABELS, 3, &pVideo->iShaderDetail);
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

void NeoSettings_Crosshair(NeoSettings *ns)
{
	static constexpr int IVIEW_ROWS = 5;
	NeoSettings::Crosshair *pCrosshair = &ns->crosshair;

	g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * IVIEW_ROWS;

	const bool bTextured = CROSSHAIR_FILES[pCrosshair->info.iStyle][0];
	NeoUI::BeginSection();
	{
		if (bTextured)
		{
			NeoSettings::Crosshair::Texture *pTex = &ns->crosshair.arTextures[pCrosshair->info.iStyle];
			vgui::surface()->DrawSetTexture(pTex->iTexId);
			vgui::surface()->DrawSetColor(pCrosshair->info.color);
			vgui::surface()->DrawTexturedRect(
				g_uiCtx.dPanel.x + g_uiCtx.iLayoutX - (pTex->iWide / 2) + (g_uiCtx.dPanel.wide / 2),
				g_uiCtx.dPanel.y + g_uiCtx.iLayoutY,
				g_uiCtx.dPanel.x + g_uiCtx.iLayoutX + (pTex->iWide / 2) + (g_uiCtx.dPanel.wide / 2),
				g_uiCtx.dPanel.y + g_uiCtx.iLayoutY + pTex->iTall);
		}
		else
		{
			const int iPreviewDynamicAccuracy = (pCrosshair->bPreviewDynamicAccuracy) ? (Max(0, (int)(sin(gpGlobals->curtime) * 24) + 16)) : 0;
			PaintCrosshair(pCrosshair->info, iPreviewDynamicAccuracy,
						   g_uiCtx.dPanel.x + g_uiCtx.iLayoutX + (g_uiCtx.dPanel.wide / 2),
						   g_uiCtx.dPanel.y + g_uiCtx.iLayoutY + (g_uiCtx.dPanel.tall / 2));
		}
		vgui::surface()->DrawSetColor(g_uiCtx.normalBgColor);

		NeoUI::SetPerRowLayout(4);
		{
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			const bool bExportPressed = NeoUI::Button(L"Export to clipboard").bPressed;
			const bool bTestCrosshairPressed = NeoUI::Button(L"Test dynamic").bPressed;
			const bool bImportPressed = NeoUI::Button(L"Import from clipboard").bPressed;
			const bool bDefaultPressed = NeoUI::Button(L"Reset to default").bPressed;
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;

			if (bExportPressed || bTestCrosshairPressed || bImportPressed)
			{
				char szClipboardCrosshair[NEO_XHAIR_SEQMAX] = {}; // zero-init
				if (bExportPressed)
				{
					// NEO NOTE (nullsystem): On Windows, SetClipboardText sets from char * looks fine
					ExportCrosshair(&pCrosshair->info, szClipboardCrosshair);
					vgui::system()->SetClipboardText(szClipboardCrosshair, V_strlen(szClipboardCrosshair));
					pCrosshair->eClipboardInfo = XHAIREXPORTNOTIFY_EXPORT_TO_CLIPBOARD;
				}
				else if (bTestCrosshairPressed)
				{
					pCrosshair->bPreviewDynamicAccuracy = !pCrosshair->bPreviewDynamicAccuracy;
				}
				else // bImportPressed
				{
					bool bImported = false;
					if (vgui::system()->GetClipboardTextCount() > 0)
					{
#ifdef WIN32
						// NEO NOTE (nullsystem): On Windows, GetClipboardText char * returns UTF-16 arranged bytes, differs from Set...
						wchar_t wszClipboardCrosshair[NEO_XHAIR_SEQMAX] = {};
						const int iClipboardWSZBytes = vgui::system()->GetClipboardText(0, wszClipboardCrosshair, NEO_XHAIR_SEQMAX);
						const int iClipboardBytes = (iClipboardWSZBytes > 0)
								? g_pVGuiLocalize->ConvertUnicodeToANSI(wszClipboardCrosshair, szClipboardCrosshair, sizeof(szClipboardCrosshair))
								: 0;
#else
						const int iClipboardBytes = vgui::system()->GetClipboardText(0, szClipboardCrosshair, NEO_XHAIR_SEQMAX);
#endif
						if (iClipboardBytes > 0)
						{
							bImported = ImportCrosshair(&pCrosshair->info, szClipboardCrosshair);
							if (bImported)
							{
								ns->bModified = true;
							}
						}
					}
					pCrosshair->eClipboardInfo = bImported
							? XHAIREXPORTNOTIFY_IMPORT_TO_CLIPBOARD
							: XHAIREXPORTNOTIFY_IMPORT_TO_CLIPBOARD_ERROR;
				}
			}

			if (bDefaultPressed)
			{
				ImportCrosshair(&pCrosshair->info, NEO_CROSSHAIR_DEFAULT);
				pCrosshair->eClipboardInfo = XHAIREXPORTNOTIFY_RESET_TO_DEFAULT;
				ns->bModified = true;
			}
		}

		NeoUI::SetPerRowLayout(1);
		{
			static constexpr const wchar_t *ARWSZ_XHAIREXPORTNOTIFY_STR[XHAIREXPORTNOTIFY__TOTAL] = {
				L"", 													// XHAIREXPORTNOTIFY_NONE
				L"Exported crosshair to clipboard", 					// XHAIREXPORTNOTIFY_EXPORT_TO_CLIPBOARD
				L"Imported crosshair from clipboard", 					// XHAIREXPORTNOTIFY_IMPORT_TO_CLIPBOARD
				L"ERROR: Unable to import crosshair from clipboard", 	// XHAIREXPORTNOTIFY_IMPORT_TO_CLIPBOARD_ERROR
				L"Crosshair reset to default",							// XHAIREXPORTNOTIFY_RESET_TO_DEFAULT
			};
			NeoUI::Label(ARWSZ_XHAIREXPORTNOTIFY_STR[pCrosshair->eClipboardInfo]);
		}
	}
	NeoUI::EndSection();

	g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * (g_iRowsInScreen - IVIEW_ROWS);
	NeoUI::BeginSection(true);
	{
		NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
		NeoUI::RingBox(L"Crosshair style", CROSSHAIR_LABELS, CROSSHAIR_STYLE__TOTAL, &pCrosshair->info.iStyle);
		NeoUI::SliderU8(L"Red", &pCrosshair->info.color[0], 0, UCHAR_MAX);
		NeoUI::SliderU8(L"Green", &pCrosshair->info.color[1], 0, UCHAR_MAX);
		NeoUI::SliderU8(L"Blue", &pCrosshair->info.color[2], 0, UCHAR_MAX);
		NeoUI::SliderU8(L"Alpha", &pCrosshair->info.color[3], 0, UCHAR_MAX);
		if (!bTextured)
		{
			NeoUI::RingBox(L"Size type", CROSSHAIR_SIZETYPE_LABELS, CROSSHAIR_SIZETYPE__TOTAL, &pCrosshair->info.iESizeType);
			switch (pCrosshair->info.iESizeType)
			{
			case CROSSHAIR_SIZETYPE_ABSOLUTE: NeoUI::SliderInt(L"Size", &pCrosshair->info.iSize, 0, CROSSHAIR_MAX_SIZE); break;
			case CROSSHAIR_SIZETYPE_SCREEN: NeoUI::Slider(L"Size", &pCrosshair->info.flScrSize, 0.0f, 1.0f, 3, 0.01f); break;
			}
			NeoUI::SliderInt(L"Thickness", &pCrosshair->info.iThick, 0, CROSSHAIR_MAX_THICKNESS);
			NeoUI::SliderInt(L"Gap", &pCrosshair->info.iGap, 0, CROSSHAIR_MAX_GAP);
			NeoUI::SliderInt(L"Outline", &pCrosshair->info.iOutline, 0, CROSSHAIR_MAX_OUTLINE);
			NeoUI::SliderInt(L"Center dot", &pCrosshair->info.iCenterDot, 0, CROSSHAIR_MAX_CENTER_DOT);
			NeoUI::RingBoxBool(L"Draw top line", &pCrosshair->info.bTopLine);
			NeoUI::SliderInt(L"Circle radius", &pCrosshair->info.iCircleRad, 0, CROSSHAIR_MAX_CIRCLE_RAD);
			NeoUI::SliderInt(L"Circle segments", &pCrosshair->info.iCircleSegments, 0, CROSSHAIR_MAX_CIRCLE_SEGMENTS);
			NeoUI::RingBox(L"Dynamic type", CROSSHAIR_DYNAMICTYPE_LABELS, CROSSHAIR_DYNAMICTYPE_TOTAL, &pCrosshair->info.iEDynamicType);
		}
		NeoUI::HeadingLabel(L"MISCELLANEOUS");
		NeoUI::RingBoxBool(L"Show other players' crosshairs", &pCrosshair->bNetworkCrosshair);
		NeoUI::RingBoxBool(L"Inaccuracy in scope", &pCrosshair->bInaccuracyInScope);
		NeoUI::RingBoxBool(L"Hip fire crosshair", &pCrosshair->bHipFireCrosshair);
	}
	NeoUI::EndSection();
}
