#pragma once

#include "tier1/convar.h"

// ConVarRef, but adds itself to a global vector
class ConVarRefEx : public ConVarRef
{
public:
	// NEO NOTE (nullsystem):
	// bExcludeGlobalPtrs prevent it to put into a global variable list of
	// ConVarRefEx. This is mostly to prevent the setting from included
	// for using to reset to default.
	// Currently only used for volume as we don't want to reset it to 100%
	// on setting default.
	ConVarRefEx(const char *pName, const bool bExcludeGlobalPtrs);
};

#define CONVARREF_DEF(_name) ConVarRefEx _name{#_name, false}
#define CONVARREF_DEFNOGLOBALPTR(_name) ConVarRefEx _name{#_name, true}

struct NeoSettings
{
	struct General
	{
		wchar_t wszNeoName[33];
		bool bOnlySteamNick;
		int iFov;
		int iViewmodelFov;
		bool bAimHold;
		bool bReloadEmpty;
		bool bViewmodelRighthand;
		bool bShowPlayerSprays;
		bool bShowPos;
		int iShowFps;
		int iDlFilter;
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
		Bind vBinds[64];
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

	General general;
	Keys keys;
	Mouse mouse;
	Audio audio;
	Video video;

	int iCurTab = 0;
	bool bBack = false;
	bool bModified = false;
	int iNextBinding = -1;

	struct CVR
	{
		// General
		CONVARREF_DEF(neo_name);
		CONVARREF_DEF(cl_onlysteamnick);
		CONVARREF_DEF(neo_fov);
		CONVARREF_DEF(neo_viewmodel_fov_offset);
		CONVARREF_DEF(neo_aim_hold);
		CONVARREF_DEF(cl_autoreload_when_empty);
		CONVARREF_DEF(cl_righthand);
		CONVARREF_DEF(cl_showpos);
		CONVARREF_DEF(cl_showfps);
		CONVARREF_DEF(hud_fastswitch);
		CONVARREF_DEF(neo_cl_toggleconsole);

		// Multiplayer
		CONVARREF_DEF(cl_player_spray_disable);
		CONVARREF_DEF(cl_download_filter);

		// Mouse
		CONVARREF_DEF(sensitivity);
		CONVARREF_DEF(m_filter);
		CONVARREF_DEF(pitch);
		CONVARREF_DEF(m_customaccel);
		CONVARREF_DEF(m_customaccel_exponent);
		CONVARREF_DEF(m_raw_input);

		// Audio
		CONVARREF_DEFNOGLOBALPTR(volume);
		CONVARREF_DEFNOGLOBALPTR(snd_musicvolume);
		CONVARREF_DEFNOGLOBALPTR(snd_victory_volume);
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
	};
	CVR cvr;
};
void NeoSettingsInit(NeoSettings *ns);
void NeoSettingsDeinit(NeoSettings *ns);
void NeoSettingsRestore(NeoSettings *ns, const NeoSettings::Keys::Flags flagsKeys = NeoSettings::Keys::NONE);
void NeoSettingsSave(const NeoSettings *ns);
void NeoSettingsResetToDefault(NeoSettings *ns);

void NeoSettings_General(NeoSettings *ns);
void NeoSettings_Keys(NeoSettings *ns);
void NeoSettings_Mouse(NeoSettings *ns);
void NeoSettings_Audio(NeoSettings *ns);
void NeoSettings_Video(NeoSettings *ns);
