#pragma once

#define CONVARREF_DEF(_name) ConVarRef _name{#_name}

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
		};
		Bind vBinds[64];
		int iBindsSize = 0;
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
		// Multiplayer
		CONVARREF_DEF(cl_playerspraydisable);
		CONVARREF_DEF(cl_downloadfilter);

		// Mouse
		CONVARREF_DEF(m_filter);
		CONVARREF_DEF(m_pitch);
		CONVARREF_DEF(m_customaccel);
		CONVARREF_DEF(m_customaccel_exponent);
		CONVARREF_DEF(m_rawinput);

		// Audio
		CONVARREF_DEF(volume);
		CONVARREF_DEF(snd_musicvolume);
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
void NeoSettingsRestore(NeoSettings *ns);
void NeoSettingsSave(const NeoSettings *ns);

void NeoSettings_General(NeoSettings *ns);
void NeoSettings_Keys(NeoSettings *ns);
void NeoSettings_Mouse(NeoSettings *ns);
void NeoSettings_Audio(NeoSettings *ns);
void NeoSettings_Video(NeoSettings *ns);
