#pragma once

#include "shareddefs.h"
#include "tier1/convar.h"
#include "neo_player_shared.h"
#include "neo_hud_crosshair.h"
#include "neo_hud_friendly_marker.h"

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

#define NEO_BINDS_TOTAL 96

// Note that this is not necessarily the same as "neo_fov" cvar max value.
// We are restricted to supporting a max of 90 due to an engine limitation.
constexpr auto maxSupportedFov = 90;
static_assert(MIN_FOV <= maxSupportedFov);
static_assert(MAX_FOV >= maxSupportedFov);

struct NeoSettings
{
	enum EquipUtilityPriorityType
	{
		EQUIP_UTILITY_PRIORITY_FRAG_SMOKE_DETPACK = 0,
		EQUIP_UTILITY_PRIORITY_CLASS_SPECIFIC,

		EQUIP_UTILITY_PRIORITY__TOTAL,
	};

	struct General
	{
		wchar_t wszNeoName[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszNeoClantag[NEO_MAX_CLANTAG_LENGTH];
		bool bOnlySteamNick;
		bool bMarkerSpecOnlyClantag;
		bool bReloadEmpty;
		bool bViewmodelRighthand;
		bool bLeanViewmodelOnly;
		int iLeanAutomatic;
		int iEquipUtilityPriority;
		bool bWeaponFastSwitch;
		bool bShowPlayerSprays;
		int iDlFilter;
		bool bStreamerMode;
		bool bAutoDetectOBS;
		bool bTachiFullAutoPreferred;
		int iBackground;
		bool bTakingDamageSounds;
	};

	struct Keys
	{
		bool bDeveloperConsole;

		struct Bind
		{
			char szBindingCmd[64];
			wchar_t wszDisplayText[64];
			ButtonCode_t bcNext;
			ButtonCode_t bcCurrent; // Only used for unbinding
			ButtonCode_t bcDefault;
			ButtonCode_t bcSecondaryNext;
			ButtonCode_t bcSecondaryCurrent;
		};
		Bind vBinds[NEO_BINDS_TOTAL];
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

	struct Controller
	{
		bool bEnabled;
		bool bReverse;
		bool bSwapSticks;
		float flSensHorizontal;
		float flSensVertical;
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
		int iFov;
		int iViewmodelFov;
		bool bSoftwareCursor;

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

	struct HUD
	{
		// Miscellaneous
		bool bShowSquadList;
		int iHealthMode;
		int iIFFVerbosity;
		bool bIFFHealthbars;
		int iObjVerbosity;
		bool bShowHints;
		bool bShowPos;
		int iShowFps;
		bool bEnableRangeFinder;
		int iExtendedKillfeed;
		bool bShowHudContextHints;
		int iKdinfoToggletype;

		// IFF Markers
		int optionChosen;
		FriendlyMarkerInfo options[NeoIFFMarkerOption::NEOIFFMARKER_OPTION_TOTAL];

#ifdef GLOWS_ENABLE
		// Player Xray
		bool bEnableXray;
		float flOutlineWidth;
		float flOutlineAlpha;
		float flCenterOpacity;
		float flTexturedOpacity;
#endif // GLOWS_ENABLE
	};

	General general;
	Keys keys;
	Mouse mouse;
	Controller controller;
	Audio audio;
	Video video;
	Crosshair crosshair;
	HUD hud;

	KeyValues* backgrounds;
	int iCBListSize;
	wchar_t** p2WszCBList;

	int iCurTab = 0;
	bool bBack = false;
	bool bModified = false;
	bool bIsValid = false;
	int iNextBinding = -1;
	bool bNextBindingSecondary = false;

	struct CVR
	{
		// General
		CONVARREF_DEF(neo_name);
		CONVARREF_DEF(neo_clantag);
		CONVARREF_DEF(cl_onlysteamnick);
		CONVARREF_DEF(neo_fov);
		CONVARREF_DEF(neo_viewmodel_fov_offset);
		CONVARREF_DEF(cl_software_cursor);
		CONVARREF_DEF(cl_autoreload_when_empty);
		CONVARREF_DEF(cl_righthand);
		CONVARREF_DEF(cl_neo_lean_viewmodel_only);
		CONVARREF_DEF(cl_neo_lean_automatic);
		CONVARREF_DEF(cl_neo_squad_hud_original);
		CONVARREF_DEF(cl_neo_hud_health_mode);
		CONVARREF_DEF(cl_neo_hud_worldpos_verbose);
		CONVARREF_DEF(cl_neo_hud_extended_killfeed);
		CONVARREF_DEF(cl_neo_tachi_prefer_auto);
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
		CONVARREF_DEF(cl_neo_hud_context_hint_enabled);
		CONVARREF_DEF(cl_neo_equip_utility_priority);
		CONVARREF_DEF(cl_neo_taking_damage_sounds);

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

		// Controller
		CONVARREF_DEF(joystick);
		CONVARREF_DEF(joy_inverty);
		CONVARREF_DEF(joy_movement_stick);
		CONVARREF_DEF(joy_yawsensitivity);
		CONVARREF_DEF(joy_pitchsensitivity);

		// Audio
		CONVARREF_DEFNOGLOBALPTR(volume);
		CONVARREF_DEFNOGLOBALPTR(snd_musicvolume);
		CONVARREF_DEFNOGLOBALPTR(snd_victory_volume);
		CONVARREF_DEFNOGLOBALPTR(snd_ping_volume);
		CONVARREF_DEF(snd_surround_speakers);
		CONVARREF_DEF(voice_modenable);
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

		// Friendly Markers
		CONVARREF_DEFNOGLOBALPTR(cl_neo_squad_marker);
		CONVARREF_DEFNOGLOBALPTR(cl_neo_friendly_marker);
		CONVARREF_DEFNOGLOBALPTR(cl_neo_spectator_marker);
#ifdef GLOWS_ENABLE
		CONVARREF_DEFNOGLOBALPTR(cl_neo_squad_xray_marker);
		CONVARREF_DEFNOGLOBALPTR(cl_neo_friendly_xray_marker);
		CONVARREF_DEFNOGLOBALPTR(cl_neo_spectator_xray_marker);

		// Xray
		CONVARREF_DEF(glow_outline_effect_enable);
		CONVARREF_DEF(glow_outline_effect_width);
		CONVARREF_DEF(glow_outline_effect_alpha);
		CONVARREF_DEF(glow_outline_effect_center_alpha);
		CONVARREF_DEF(glow_outline_effect_textured_center_alpha);
#endif // GLOWS_ENABLE
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
void NeoSettingsEndVoiceTweakMode();

void NeoSettings_General(NeoSettings *ns);
void NeoSettings_Keys(NeoSettings *ns);
void NeoSettings_MouseController(NeoSettings *ns);
void NeoSettings_Audio(NeoSettings *ns);
void NeoSettings_Video(NeoSettings *ns);
void NeoSettings_Crosshair(NeoSettings *ns);
void NeoSettings_HUD(NeoSettings *ns);
