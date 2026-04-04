#include "neo_mp3player.h"

#include "filesystem.h"
#include "threadtools.h"

#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/Controls.h>

#include "neo_misc.h"
#include "ui/neo_utils.h"

#include "miniaudio.h"

namespace NeoMP3
{

static void ConCmdCallback()
{
	NeoMP3::State *mps = NeoMP3::GetState();
	mps->flagsPlayStateNext = (mps->bPlaying)
			? NeoMP3::PLAYSTATE_FLAG_PAUSED : NeoMP3::PLAYSTATE_FLAG_PLAY;
	Update();
}

ConCommand neo_mp3("neo_mp3", ConCmdCallback, "NEO Radio play/pause toggle", FCVAR_DONTRECORD);

ConVar cl_neo_radio_shuffle("cl_neo_radio_shuffle", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Randomize NEO Radio song order", true, 0.0f, true, 1.0f);
ConVar cl_neo_radio_volume_separate_ingame("cl_neo_radio_volume_separate_ingame", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD, "NEO Radio song volume is separate in game", true, 0.0f, true, 1.0f);
ConVar cl_neo_radio_volume_ingame("cl_neo_radio_volume_ingame", "0.2", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD, "NEO Radio song volume in game", true, 0.0f, true, 1.0f);
ConVar cl_neo_radio_pause_ingame("cl_neo_radio_pause_ingame", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD, "NEO Radio pauses when going in game, unpause when in menu", true, 0.0f, true, 1.0f);

ConVar cl_neo_radio_startup("cl_neo_radio_startup", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Which song NEO Radio will pick on startup - 0 = OG:NT Default, 1 = Random, 2 = First or Shuffle", true, 0.0f, true, static_cast<float>(STARTUP_TYPE__LAST));
static constexpr const char DEFAULT_STARTUP_FILENAME_MP3[] = "112 - out.mp3";

static void MaMP3MetaCallback(void *pUserData, const ma_dr_mp3_metadata *pMetadata)
{
	if (!pMetadata || pMetadata->rawDataSize <= 0)
	{
		return;
	}

	// This is called-back multiple time over different metadata formats,
	// so do not clear anything here
	NeoMP3::Song *pSong = (NeoMP3::Song *)(pUserData);
	const char *pszRawData = (char *)(pMetadata->pRawData);

	// NEO NOTE (nullsystem): At the moment we only deal with the NT soundtrack that
	// OG:NT directory provides, and those have ID3v1 tags that's good enough for now
	// and a very simple implementation compared to other metadata formats
	if (MA_DR_MP3_METADATA_TYPE_ID3V1 == pMetadata->type && pMetadata->rawDataSize >= 63)
	{
		// See: https://id3.org/ID3v1

		// 'TAG' - 3 bytes
		// Title/song name - 30 bytes
		g_pVGuiLocalize->ConvertANSIToUnicode(pszRawData + 3,
				pSong->wszTitle, sizeof(pSong->wszTitle));
		// Artist - 30 bytes
		g_pVGuiLocalize->ConvertANSIToUnicode(pszRawData + 3 + 30,
				pSong->wszArtist, sizeof(pSong->wszArtist));
	}
}

class CNeoMP3Thread : public CWorkerThread
{
public:
	NeoMP3::State m_state = {};

	enum
	{
		UPDATE,
		EXIT,
	};

	int Run()
	{
		ma_engine maEngine = {};
		ma_result maResult = ma_engine_init(NULL, &maEngine);
		if (maResult != MA_SUCCESS)
		{
			Msg("NEO MP3 Player error: Failed to initalize engine!");
			return 1;
		}

		m_state.iSongsSize = 0;
		// Initialize music list
		{
			ma_dr_mp3 maMP3 = {};

			FileFindHandle_t findHdl;
			for (const char *pszFilename = filesystem->FindFirstEx("soundtrack/*.mp3", "GAME", &findHdl);
					pszFilename && m_state.iSongsSize < MAX_SONGS;
					pszFilename = filesystem->FindNext(findHdl))
			{
				char szRelPath[MAX_PATH];
				V_sprintf_safe(szRelPath, "soundtrack/%s", pszFilename);

				NeoMP3::Song *pSong = &m_state.songs[m_state.iSongsSize];
				filesystem->RelativePathToFullPath_safe(szRelPath, "GAME",
						pSong->szPath);

				// Fallback filename as title
				g_pVGuiLocalize->ConvertANSIToUnicode(pszFilename,
						pSong->wszTitle, sizeof(pSong->wszTitle));

				if (ma_dr_mp3_init_file_with_metadata(&maMP3, pSong->szPath,
							MaMP3MetaCallback, (void *)(pSong), nullptr))
				{
					ma_dr_mp3_uninit(&maMP3);

					if (STARTUP_TYPE_DEFAULT == cl_neo_radio_startup.GetInt()
							&& 0 == V_strcmp(DEFAULT_STARTUP_FILENAME_MP3, pszFilename))
					{
						m_state.iCurIdx = m_state.iSongsSize;
					}

					++m_state.iSongsSize;
				}
			}
			filesystem->FindClose(findHdl);
		}

		if (m_state.iSongsSize <= 0)
		{
			ma_engine_uninit(&maEngine);
			return 2;
		}

		ConVarRef cvr_snd_musicvolume("snd_musicvolume");
		ConVarRef cvr_snd_mute_losefocus("snd_mute_losefocus");

		bool bInitSound = false;
		ma_sound maSound = {};
		unsigned nCall = 0;

		if (STARTUP_TYPE_RANDOM == cl_neo_radio_startup.GetInt()
				|| (STARTUP_TYPE_SHUFFLETYPE == cl_neo_radio_startup.GetInt()
					&& cl_neo_radio_shuffle.GetBool()))
		{
			m_state.iCurIdx = RandomInt(0, m_state.iSongsSize - 1);
		}
		const int iFirstIdx = m_state.iCurIdx;

		if (cl_neo_radio_shuffle.GetBool())
		{
			CreateShuffle();
		}

		while (1)
		{
			WaitForCall(250, &nCall);
			if (nCall == EXIT)
			{
				Reply(1);
				break;
			}

			const float flVol =
					(cvr_snd_mute_losefocus.GetBool() && false == engine->IsActiveApp())
						? 0.0f
						: (cl_neo_radio_volume_separate_ingame.GetBool() && IsInGame())
							? cl_neo_radio_volume_ingame.GetFloat()
							: cvr_snd_musicvolume.GetFloat();

			if (flVol != ma_engine_get_volume(&maEngine))
			{
				ma_engine_set_volume(&maEngine, flVol);
			}

			if (cl_neo_radio_pause_ingame.GetBool()
					&& PLAYSTATE_FLAG_NIL == m_state.flagsPlayStateNext)
			{
				const bool bNowInGame = IsInGame();
				if (m_state.bInGame && false == bNowInGame)
				{
					m_state.flagsPlayStateNext = PLAYSTATE_FLAG_PLAY | PLAYSTATE_FLAG_SONGNEXT;
				}
				else if (false == m_state.bInGame && bNowInGame)
				{
					// Fade out pause current playing music
					ma_sound_stop_with_fade_in_milliseconds(&maSound, 1000);
				}
				m_state.bInGame = bNowInGame;
			}

			// Fetch current state straight from ma_sound_...
			bool bNowPlaying = ma_sound_is_playing(&maSound);
			float flNowSecsCursor, flNowSecsLength;
			ma_sound_get_cursor_in_seconds(&maSound, &flNowSecsCursor);
			ma_sound_get_length_in_seconds(&maSound, &flNowSecsLength);

			if (ma_sound_at_end(&maSound)
					|| false == bInitSound
					|| (m_state.flagsPlayStateNext & (
							  PLAYSTATE_FLAG_SONGCHANGE
							| PLAYSTATE_FLAG_SONGNEXT
							| PLAYSTATE_FLAG_SONGPREVIOUS)))
			{
				if (false == (m_state.flagsPlayStateNext & PLAYSTATE_FLAG_SONGCHANGE))
				{
					const int iIdxDiff =
							(m_state.flagsPlayStateNext & PLAYSTATE_FLAG_SONGPREVIOUS) ? -1 : +1;
					if (cl_neo_radio_shuffle.GetBool())
					{
						// In startup, first should in shuffle should already been == iFirstIdx
						m_state.iCurShuffleIdx = (bInitSound)
								? LoopAroundInArray(m_state.iCurShuffleIdx + iIdxDiff, m_state.iSongsSize)
								: 0;
						m_state.iCurIdx = m_state.iaShuffleIdx[m_state.iCurShuffleIdx];
					}
					else
					{
						m_state.iCurIdx = (bInitSound)
								? LoopAroundInArray(m_state.iCurIdx + iIdxDiff, m_state.iSongsSize)
								: iFirstIdx;
					}
				}

				if (bInitSound)
				{
					ma_sound_uninit(&maSound);
				}
				maResult = ma_sound_init_from_file(&maEngine, m_state.songs[m_state.iCurIdx].szPath,
						MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, nullptr, nullptr, &maSound);
				bInitSound = (maResult == MA_SUCCESS);
				if (bInitSound)
				{
					m_state.flagsPlayStateNext |= PLAYSTATE_FLAG_PLAY;
					m_state.flSecsCursor = 0.0f;
				}
			}

			if (m_state.flagsPlayStateNext & PLAYSTATE_FLAG_PLAY)
			{
				ma_sound_reset_stop_time_and_fade(&maSound);
				ma_sound_start(&maSound);
				bNowPlaying = true;
			}
			if (m_state.flagsPlayStateNext & PLAYSTATE_FLAG_PAUSED)
			{
				ma_sound_stop(&maSound);
				bNowPlaying = false;
			}

			if (m_state.flagsPlayStateNext & (PLAYSTATE_FLAG_PLAY | PLAYSTATE_FLAG_CURSOR))
			{
				ma_sound_seek_to_second(&maSound, m_state.flSecsCursor);
				flNowSecsCursor = m_state.flSecsCursor;
			}

			// Update this state
			m_state.flagsPlayStateNext = PLAYSTATE_FLAG_NIL;
			m_state.bPlaying = bNowPlaying;
			if (m_state.bPlaying)
			{
				m_state.flSecsCursor = flNowSecsCursor;
				m_state.flSecsLength = flNowSecsLength;
			}

			Reply(1);
		}

		if (bInitSound) ma_sound_uninit(&maSound);
		ma_engine_uninit(&maEngine);
		return 0;
	}
};

static inline CNeoMP3Thread g_NeoMP3Thread;

void Init()
{
	if (!g_NeoMP3Thread.IsAlive())
	{
		g_NeoMP3Thread.Start();
	}
}

void Deinit()
{
	g_NeoMP3Thread.CallWorker(CNeoMP3Thread::EXIT);
}

void Update()
{
	g_NeoMP3Thread.CallWorker(CNeoMP3Thread::UPDATE);
}

void MusicVolCallback(IConVar *, const char *, float)
{
	Update();
}

State *GetState()
{
	return &g_NeoMP3Thread.m_state;
}

void CreateShuffle()
{
	NeoMP3::State *mps = NeoMP3::GetState();

	// Very first shuffle index always the
	// current playing index
	mps->iaShuffleIdx[0] = mps->iCurIdx;
	for (int i = 1; i < mps->iSongsSize; ++i)
	{
		bool bIsDupe = false;
		int iPickIdx = RandomInt(0, mps->iSongsSize - 1);
		do
		{
			bIsDupe = false;
			for (int j = 0; j < i; ++j)
			{
				if (mps->iaShuffleIdx[j] == iPickIdx)
				{
					bIsDupe = true;
					break;
				}
			}
			if (bIsDupe)
			{
				iPickIdx = LoopAroundInArray(iPickIdx + ((i % 2 == 0) ? -1 : +1), mps->iSongsSize);
			}
		} while (bIsDupe);

		mps->iaShuffleIdx[i] = iPickIdx;
	}
}

} // end namespace NeoMP3

