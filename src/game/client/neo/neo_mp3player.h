#pragma once

class IConVar;

namespace NeoMP3
{

enum StartupType
{
	STARTUP_TYPE_DEFAULT = 0,	// Find "Out" song (OG:NT's main menu song)
	STARTUP_TYPE_RANDOM,		// Random first song
	STARTUP_TYPE_SHUFFLETYPE,	// First song or shuffle's first song

	STARTUP_TYPE__TOTAL,
	STARTUP_TYPE__LAST = STARTUP_TYPE__TOTAL - 1,
};

enum PlayStateFlag_
{
	PLAYSTATE_FLAG_NIL 			= 0,
	PLAYSTATE_FLAG_PLAY 		= 1 << 0,
	PLAYSTATE_FLAG_PAUSED 		= 1 << 1,
	PLAYSTATE_FLAG_CURSOR 		= 1 << 2,	// Set flSecsCursor
	PLAYSTATE_FLAG_SONGCHANGE 	= 1 << 3,	// Set iCurIdx, direct song change
	PLAYSTATE_FLAG_SONGNEXT 	= 1 << 4,	// Next direct/shuffle
	PLAYSTATE_FLAG_SONGPREVIOUS	= 1 << 5,	// Previous direct/shuffle
};
typedef int PlayStateFlags;

static constexpr int MAX_SONGS = 32;

struct Song
{
	char szPath[MAX_PATH];
	wchar_t wszTitle[30 + 1];
	wchar_t wszArtist[30 + 1];
};

struct State
{
	// Current playing audio info
	float flSecsCursor;
	float flSecsLength;
	bool bPlaying;

	// Shuffle feature
	int iCurShuffleIdx;
	int iaShuffleIdx[MAX_SONGS];

	// Songs list + current song info
	int iCurIdx;
	int iSongsSize;
	Song songs[MAX_SONGS];

	// Requests from user to player
	PlayStateFlags flagsPlayStateNext;

	// Extra infos
	bool bInGame;
};

void Init();
void Deinit();
void Update();
void MusicVolCallback(IConVar *, const char *, float);
State *GetState();
void CreateShuffle();

extern ConVar cl_neo_radio_shuffle;

} // end namespace NeoMP3

