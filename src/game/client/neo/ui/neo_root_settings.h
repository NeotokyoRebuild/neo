#pragma once

#include "tier1/convar.h"
#include "neo_player_shared.h"
#include "neo_hud_crosshair.h"

// NEO TODO (nullsystem): Implement our own file IO dialog
#include "vgui_controls/FileOpenDialog.h"

// ConVarRef, but adds itself to a global vector
class ConVarRefEx : public ConVarRef
{
public:
	// NEO NOTE (nullsystem):
	// bExcludeGlobalPtrs prevent it to put into a global variable list of
	// ConVarRefEx. This is mostly to prevent the setting from included
	// for using to reset to default.
	// Currently only used for volume as we don't want to reset it to 100%
	// on setting default, and crosshair as has its own default reset button.
	ConVarRefEx(const char *pName, const bool bExcludeGlobalPtrs);
};

#define CONVARREF_DEF(_name) ConVarRefEx _name{#_name, false}
#define CONVARREF_DEFNOGLOBALPTR(_name) ConVarRefEx _name{#_name, true}

enum XHairExportNotify
{
	XHAIREXPORTNOTIFY_NONE = 0,
	XHAIREXPORTNOTIFY_EXPORT_TO_CLIPBOARD,
	XHAIREXPORTNOTIFY_IMPORT_TO_CLIPBOARD,
	XHAIREXPORTNOTIFY_IMPORT_TO_CLIPBOARD_ERROR,
	XHAIREXPORTNOTIFY_RESET_TO_DEFAULT,

	XHAIREXPORTNOTIFY__TOTAL,
};

struct NeoSettings
{
	struct General
	{
		wchar_t wszNeoName[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszNeoClantag[NEO_MAX_CLANTAG_LENGTH];
		bool bOnlySteamNick;
		bool bMarkerSpecOnlyClantag;
		int iFov;
		int iViewmodelFov;
		bool bAimHold;
		bool bReloadEmpty;
		bool bViewmodelRighthand;
		bool bLeanViewmodelOnly;
		bool bHipFireCrosshair;
		int iLeanAutomatic;
		bool bShowSquadList;
		bool bShowPlayerSprays;
		bool bShowHints;
		bool bShowPos;
		int iShowFps;
		int iDlFilter;
		bool bStreamerMode;
		bool bAutoDetectOBS;
		bool bEnableRangeFinder;
		bool bExtendedKillfeed;
		int iBackground;
		int iKdinfoToggletype;
	};

	struct Keys
	{
		bool bWeaponFastSwitch;
		bool bDeveloperConsole;

		struct Bind
		{
			char szBindingCmd[64];
			wchar_t wszDisplayText[64];
			ButtonCode_t bcNext;
			ButtonCode_t bcCurrent; // Only used for unbinding
			ButtonCode_t bcDefault;
		};
		Bind vBinds[96];
		int iBindsSize = 0;

		// Will be checked often so cached
		ButtonCode_t bcConsole;

		enum Flags
		{
			NONE = 0,
			SKIP_KEYS = 1,
		};
	};

	struct Mouse
	{
		float flSensitivity;
		float flZoomSensitivityRatio;
		bool bRawInput;
		bool bFilter;
		bool bReverse;
		bool bCustomAccel;
		float flExponent;
	};

	struct Audio
	{
		float flVolMain;
		float flVolMusic;
		float flVolVictory;
		float flVolPing;
		int iSoundSetup;
		int iSoundQuality;
		bool bMuteAudioUnFocus;
		bool bVoiceEnabled;
		float flVolVoiceRecv;
		bool bMicBoost;

		// Microphone tweaking
		float flSpeakingVol;
		float flLastFetchInterval;
	};

	struct Video
	{
		int iResolution;
		int iWindow;
		int iCoreRendering;
		int iModelDetail;
		int iTextureDetail;
		int iShaderDetail;
		int iWaterDetail;
		int iShadowDetail;
		bool bColorCorrection;
		int iAntiAliasing;
		int iFilteringMode;
		bool bVSync;
		bool bMotionBlur;
		int iHDR;
		float flGamma;

		// Video modes
		int iVMListSize;
		vmode_t *vmList;
		wchar_t **p2WszVmDispList;
	};

	struct Crosshair
	{
		CrosshairInfo info;
		XHairExportNotify eClipboardInfo;
		bool bNetworkCrosshair;
		bool bInaccuracyInScope;
		bool bHipFireCrosshair;
		bool bPreviewDynamicAccuracy;

		// Textures
		struct Texture
		{
			int iTexId;
			int iWide;
			int iTall;
		};
		Texture arTextures[CROSSHAIR_STYLE__TOTAL];
	};

	General general;
	Keys keys;
	Mouse mouse;
	Audio audio;
	Video video;
	Crosshair crosshair;

	KeyValues* backgrounds;
	int iCBListSize;
	wchar_t** p2WszCBList;

	int iCurTab = 0;
	bool bBack = false;
	bool bModified = false;
	bool bIsValid = false;
	int iNextBinding = -1;

	struct CVR
	{
		// General
		CONVARREF_DEF(neo_name);
		CONVARREF_DEF(neo_clantag);
		CONVARREF_DEF(cl_onlysteamnick);
		CONVARREF_DEF(cl_neo_clantag_friendly_marker_spec_only);
		CONVARREF_DEF(neo_fov);
		CONVARREF_DEF(neo_viewmodel_fov_offset);
		CONVARREF_DEF(neo_aim_hold);
		CONVARREF_DEF(cl_autoreload_when_empty);
		CONVARREF_DEF(cl_righthand);
		CONVARREF_DEF(cl_neo_lean_viewmodel_only);
		CONVARREF_DEF(cl_neo_lean_automatic);
		CONVARREF_DEF(cl_neo_squad_hud_original);
		CONVARREF_DEF(cl_neo_hud_extended_killfeed);
		CONVARREF_DEF(cl_neo_showhints);
		CONVARREF_DEF(cl_showpos);
		CONVARREF_DEF(cl_showfps);
		CONVARREF_DEF(hud_fastswitch);
		CONVARREF_DEF(cl_neo_toggleconsole);
		CONVARREF_DEF(cl_neo_streamermode);
		CONVARREF_DEF(cl_neo_streamermode_autodetect_obs);
		CONVARREF_DEF(cl_neo_hud_rangefinder_enabled);
		CONVARREF_DEF(sv_unlockedchapters);
		CONVARREF_DEF(cl_neo_kdinfo_toggletype);

		// Multiplayer
		CONVARREF_DEF(cl_spraydisable);
		CONVARREF_DEF(cl_downloadfilter);

		// Mouse
		CONVARREF_DEF(sensitivity);
		CONVARREF_DEF(zoom_sensitivity_ratio);
		CONVARREF_DEF(m_filter);
		CONVARREF_DEF(m_pitch);
		CONVARREF_DEF(m_customaccel);
		CONVARREF_DEF(m_customaccel_exponent);
		CONVARREF_DEF(m_rawinput);

		// Audio
		CONVARREF_DEFNOGLOBALPTR(volume);
		CONVARREF_DEFNOGLOBALPTR(snd_musicvolume);
		CONVARREF_DEFNOGLOBALPTR(snd_victory_volume);
		CONVARREF_DEFNOGLOBALPTR(snd_ping_volume);
		CONVARREF_DEF(snd_surround_speakers);
		CONVARREF_DEF(voice_enable);
		CONVARREF_DEF(voice_scale);
		CONVARREF_DEF(snd_mute_losefocus);
		CONVARREF_DEF(snd_pitchquality);
		CONVARREF_DEF(dsp_slow_cpu);

		// Video
		CONVARREF_DEF(mat_queue_mode);
		CONVARREF_DEF(r_rootlod);
		CONVARREF_DEF(mat_picmip);
		CONVARREF_DEF(mat_reducefillrate);
		CONVARREF_DEF(r_lightmap_bicubic);
		CONVARREF_DEF(r_waterforceexpensive);
		CONVARREF_DEF(r_waterforcereflectentities);
		CONVARREF_DEF(r_flashlightdepthtexture);
		CONVARREF_DEF(r_shadowrendertotexture);
		CONVARREF_DEF(mat_colorcorrection);
		CONVARREF_DEF(mat_antialias);
		CONVARREF_DEF(mat_trilinear);
		CONVARREF_DEF(mat_forceaniso);
		CONVARREF_DEF(mat_vsync);
		CONVARREF_DEF(mat_motion_blur_enabled);
		CONVARREF_DEF(mat_hdr_level);
		CONVARREF_DEF(mat_monitorgamma);

		// Crosshair
		CONVARREF_DEFNOGLOBALPTR(cl_neo_crosshair);
		CONVARREF_DEF(cl_neo_crosshair_network);
		CONVARREF_DEF(cl_neo_crosshair_scope_inaccuracy);
		CONVARREF_DEF(cl_neo_crosshair_hip_fire);
	};
	CVR cvr;
};
void NeoSettingsInit(NeoSettings *ns);
void NeoSettingsBackgroundsInit(NeoSettings* ns);
void NeoSettingsBackgroundWrite(const NeoSettings* ns, const char* backgroundName = nullptr);
void NeoSettingsDeinit(NeoSettings *ns);
void NeoSettingsRestore(NeoSettings *ns, const NeoSettings::Keys::Flags flagsKeys = NeoSettings::Keys::NONE);
void NeoSettingsSave(const NeoSettings *ns);
void NeoSettingsResetToDefault(NeoSettings *ns);

void NeoSettings_General(NeoSettings *ns);
void NeoSettings_Keys(NeoSettings *ns);
void NeoSettings_Mouse(NeoSettings *ns);
void NeoSettings_Audio(NeoSettings *ns);
void NeoSettings_Video(NeoSettings *ns);
void NeoSettings_Crosshair(NeoSettings *ns);
