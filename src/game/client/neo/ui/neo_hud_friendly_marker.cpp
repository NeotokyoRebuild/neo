#include "cbase.h"
#include "neo_hud_friendly_marker.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include <engine/ivdebugoverlay.h>
#include "ienginevgui.h"

#include "neo_gamerules.h"
#include "c_neo_player.h"

#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_FriendlyMarker, NHudFriendlyMarker);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(FriendlyMarker, 0.01)

void iffMarkerChangeCallback( IConVar *pConVar, char const* pOldString, float flOldValue [[maybe_unused]])
{
	CNEOHud_FriendlyMarker* iffHudElement = GET_NAMED_HUDELEMENT(CNEOHud_FriendlyMarker, NHudFriendlyMarker);
	if (!iffHudElement)
	{
		return;
	}
	
	ConVarRef conVar( pConVar );
	const char* newValue = conVar.GetString();

	NeoIFFMarkerOption conVarChanged = NEOIFFMARKER_OPTION_SQUAD;
	if (FStrEq(pConVar->GetName(), "cl_neo_friendly_marker")) { conVarChanged = NEOIFFMARKER_OPTION_FRIENDLY; }
	else if (FStrEq(pConVar->GetName(), "cl_neo_spectator_marker")) { conVarChanged = NEOIFFMARKER_OPTION_SPECTATOR; }
#ifdef GLOWS_ENABLE
	else if (FStrEq(pConVar->GetName(), "cl_neo_friendly_xray_marker")) { conVarChanged = NEOIFFMARKER_OPTION_FRIENDLY_XRAY; }
	else if (FStrEq(pConVar->GetName(), "cl_neo_squad_xray_marker")) { conVarChanged = NEOIFFMARKER_OPTION_SQUAD_XRAY; }
	else if (FStrEq(pConVar->GetName(), "cl_neo_spectator_xray_marker")) { conVarChanged = NEOIFFMARKER_OPTION_SPECTATOR_XRAY; }
#endif // GLOWS_ENABLE
	
	if (!ImportMarker(&iffHudElement->m_szMarkerSettings[conVarChanged], newValue))
	{ // unsuccessfull, revert value
		pConVar->SetValue(pOldString);
	}
}

ConVar cl_neo_squad_marker("cl_neo_squad_marker", NEO_SQUAD_MARKER_DEFAULT, FCVAR_ARCHIVE | FCVAR_DONTRECORD, "IFF Marker settings for squad-mates", iffMarkerChangeCallback);
ConVar cl_neo_friendly_marker("cl_neo_friendly_marker", NEO_FRIENDLY_MARKER_DEFAULT, FCVAR_ARCHIVE | FCVAR_DONTRECORD, "IFF Marker settings for team-mates", iffMarkerChangeCallback);
ConVar cl_neo_spectator_marker("cl_neo_spectator_marker", NEO_SPECTATOR_MARKER_DEFAULT, FCVAR_ARCHIVE | FCVAR_DONTRECORD, "IFF Marker settings for spectated players", iffMarkerChangeCallback);
#ifdef GLOWS_ENABLE
ConVar cl_neo_squad_xray_marker("cl_neo_squad_xray_marker", NEO_SQUAD_XRAY_MARKER_DEFAULT, FCVAR_ARCHIVE | FCVAR_DONTRECORD, "IFF Marker settings for squad-mates with xray enabled", iffMarkerChangeCallback);
ConVar cl_neo_friendly_xray_marker("cl_neo_friendly_xray_marker", NEO_FRIENDLY_XRAY_MARKER_DEFAULT, FCVAR_ARCHIVE | FCVAR_DONTRECORD, "IFF Marker settings for team-mates with xray enabled", iffMarkerChangeCallback);
ConVar cl_neo_spectator_xray_marker("cl_neo_spectator_xray_marker", NEO_SPECTATOR_XRAY_MARKER_DEFAULT, FCVAR_ARCHIVE | FCVAR_DONTRECORD, "IFF Marker settings for spectated players with xray enabled", iffMarkerChangeCallback);
#endif // GLOWS_ENABLE

void CNEOHud_FriendlyMarker::UpdateStateForNeoHudElementDraw()
{
}

CNEOHud_FriendlyMarker::CNEOHud_FriendlyMarker(const char* pElemName, vgui::Panel* parent)
	: CNEOHud_WorldPosMarker(pElemName, parent)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_FRIENDLY_MARKER;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	m_hStarTex = vgui::surface()->CreateNewTextureID();
	Assert(m_hStarTex > 0);
	vgui::surface()->DrawSetTextureFile(m_hStarTex, "vgui/hud/star", 1, false);
	vgui::surface()->DrawGetTextureSize(m_hStarTex, m_iIconWidth, m_iIconHeight);

	m_hNonStarTex = vgui::surface()->CreateNewTextureID();
	Assert(m_hNonStarTex > 0);
	vgui::surface()->DrawSetTextureFile(m_hNonStarTex, "vgui/hud/non_star", 1, false);

	m_hUniqueTex = vgui::surface()->CreateNewTextureID();
	Assert(m_hUniqueTex > 0);
	vgui::surface()->DrawSetTextureFile(m_hUniqueTex, "vgui/hud/unique_star", 1, false);

	ImportMarker(&m_szMarkerSettings[NEOIFFMARKER_OPTION_SQUAD], cl_neo_squad_marker.GetString());
	ImportMarker(&m_szMarkerSettings[NEOIFFMARKER_OPTION_FRIENDLY], cl_neo_friendly_marker.GetString());
	ImportMarker(&m_szMarkerSettings[NEOIFFMARKER_OPTION_SPECTATOR], cl_neo_spectator_marker.GetString());
#ifdef GLOWS_ENABLE
	ImportMarker(&m_szMarkerSettings[NEOIFFMARKER_OPTION_SQUAD_XRAY], cl_neo_squad_xray_marker.GetString());
	ImportMarker(&m_szMarkerSettings[NEOIFFMARKER_OPTION_FRIENDLY_XRAY], cl_neo_friendly_xray_marker.GetString());
	ImportMarker(&m_szMarkerSettings[NEOIFFMARKER_OPTION_SPECTATOR_XRAY], cl_neo_spectator_xray_marker.GetString());
#endif // GLOWS_ENABLE

	SetVisible(true);
}

void CNEOHud_FriendlyMarker::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_FriendlyMarker::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmaller", true);
}

void CNEOHud_FriendlyMarker::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	C_NEO_Player* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer)
	{
		return;
	}
	
	C_Team* team = localPlayer->GetTeam();
	if (!team)
	{
		return;
	}

	const bool isSpectator = team->GetTeamNumber() == TEAM_SPECTATOR;
	const auto *pTargetPlayer = (localPlayer->GetObserverMode() == OBS_MODE_IN_EYE) ?
				dynamic_cast<C_NEO_Player *>(localPlayer->GetObserverTarget()) : nullptr;
	
	if (NEORules()->IsTeamplay())
	{
		if (isSpectator)
		{
			auto nsf = GetGlobalTeam(TEAM_NSF);
			DrawPlayerForTeam(nsf, localPlayer, pTargetPlayer);

			auto jinrai = GetGlobalTeam(TEAM_JINRAI);
			DrawPlayerForTeam(jinrai, localPlayer, pTargetPlayer);
		}
		else
		{
			DrawPlayerForTeam(team, localPlayer, pTargetPlayer);
		}
	}
	else
	{
		if (isSpectator)
		{
			// TODO: They're not really Jinrai/NSF?
			auto nsf = GetGlobalTeam(TEAM_NSF);
			DrawPlayerForTeam(nsf, localPlayer, pTargetPlayer);

			auto jinrai = GetGlobalTeam(TEAM_JINRAI);
			DrawPlayerForTeam(jinrai, localPlayer, pTargetPlayer);
		}
	}
}

void CNEOHud_FriendlyMarker::DrawPlayerForTeam(C_Team *team, const C_NEO_Player *localPlayer,
											   const C_NEO_Player *pTargetPlayer) const
{
	auto memberCount = team->GetNumPlayers();
	auto teamColour = GetTeamColour(team->GetTeamNumber());
	for (int i = 0; i < memberCount; ++i)
	{
		auto player = static_cast<C_NEO_Player *>(team->GetPlayer(i));
		if (player && localPlayer->entindex() != player->entindex() && player->IsAlive())
		{
			if (pTargetPlayer && player->entindex() == pTargetPlayer->entindex())
			{
				// NEO NOTE (nullsystem): Skip this if we're observing this player in first person
				continue;
			}
			DrawPlayer(teamColour, player, localPlayer);
		}		
	}
}

#ifdef GLOWS_ENABLE
extern ConVar glow_outline_effect_enable;
#endif // GLOWS_ENABLE
extern ConVar cl_neo_hud_health_mode;
void CNEOHud_FriendlyMarker::DrawPlayer(Color teamColor, C_NEO_Player *player, const C_NEO_Player *localPlayer) const
{
	NeoIFFMarkerOption markerUsedIndex = NEOIFFMARKER_OPTION_FRIENDLY;
	if (localPlayer->IsObserver()){
#ifdef GLOWS_ENABLE
		if (glow_outline_effect_enable.GetBool()) { markerUsedIndex = NEOIFFMARKER_OPTION_SPECTATOR_XRAY; } else { 
#endif //  GLOWS_ENABLE
			markerUsedIndex = NEOIFFMARKER_OPTION_SPECTATOR;
#ifdef GLOWS_ENABLE
		}
#endif //  GLOWS_ENABLE
	}
	else if (player->GetStar() == localPlayer->GetStar()) {
#ifdef GLOWS_ENABLE
		if (glow_outline_effect_enable.GetBool()) {	markerUsedIndex = NEOIFFMARKER_OPTION_SQUAD_XRAY; } else { 
#endif //  GLOWS_ENABLE
			markerUsedIndex = NEOIFFMARKER_OPTION_SQUAD;
#ifdef GLOWS_ENABLE
		}
#endif //  GLOWS_ENABLE
	}
#ifdef GLOWS_ENABLE
	else if (glow_outline_effect_enable.GetBool()) { markerUsedIndex = NEOIFFMARKER_OPTION_FRIENDLY_XRAY; }	
#endif //  GLOWS_ENABLE
	const FriendlyMarkerInfo* settings = &m_szMarkerSettings[markerUsedIndex];

	const Vector pos = player->GetAbsOrigin() + Vector(0, 0, player->GetPlayerMaxs().z + settings->iInitialOffset);
	int x, y;
	if (!GetVectorInScreenSpace(pos, x, y, nullptr)) {
		return;
	}

	char textASCII[MAX_MARKER_STRSIZE];
	int textSize = 0;

	auto DisplayText = [this, &textSize, x](const char *textASCII, const int y, const int maxLength) {
			if (maxLength <= 0)
			{
				textSize = 0;
				return;
			}

			wchar_t textUTF[MAX_MARKER_STRSIZE];
			COMPILE_TIME_ASSERT(sizeof(textUTF) == sizeof(wchar_t) * MAX_MARKER_STRSIZE);
			textUTF[0] = L'\0';
			const int numChars = g_pVGuiLocalize->ConvertANSIToUnicode(textASCII, textUTF, narrow_cast<int>(Min(sizeof(textUTF), sizeof(wchar_t) * maxLength)));
			if (numChars <= 0)
			{
				textSize = 0;
				return;
			}

			int textWidth, textHeight;
			vgui::surface()->GetTextSize(m_hFont, textUTF, textWidth, textHeight);
			vgui::surface()->DrawSetTextPos(x - (textWidth / 2), y - textHeight);
			vgui::surface()->DrawPrintText(textUTF, numChars);
			textSize = textHeight;
		};

	vgui::surface()->DrawSetTextFont(m_hFont);
	vgui::surface()->DrawSetTextColor(teamColor);

	if (settings->bShowDistance) {
		const float flDistance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(localPlayer->GetAbsOrigin());
		if (settings->bVerboseDistance) {
			V_snprintf(textASCII, MAX_MARKER_STRSIZE, "%s: %.0fm",
				player->IsCarryingGhost() ? "GHOST DISTANCE" : "DISTANCE",
				flDistance);
		}
		else {
			V_snprintf(textASCII, MAX_MARKER_STRSIZE, "%.0fm",
				flDistance);
		}
		DisplayText(textASCII, y, MAX_MARKER_STRSIZE);
		y -= textSize;
	}

	const bool unique = player->GetClass() == NEO_CLASS_JUGGERNAUT || player->GetClass() == NEO_CLASS_VIP;
	if (settings->bShowSquadMarker || unique) {
		if (unique) {
			vgui::surface()->DrawSetTexture(m_hUniqueTex);
		}
		else if (player->GetStar() == localPlayer->GetStar()) {
			vgui::surface()->DrawSetTexture(m_hStarTex);
		}
		else {
			vgui::surface()->DrawSetTexture(m_hNonStarTex);
		}
		
		vgui::surface()->DrawSetColor(teamColor);
		vgui::surface()->DrawTexturedRect(
			x - (m_iIconWidth * settings->flSquadMarkerScale * 0.5),
			y - m_iIconHeight * settings->flSquadMarkerScale,
			x + (m_iIconWidth * settings->flSquadMarkerScale * 0.5),
			y
		);
		y -= m_iIconHeight * settings->flSquadMarkerScale;
	}

	const float fadeTextMultiplier = GetFadeValueTowardsScreenCentre(x, y);
	if (fadeTextMultiplier < 0.001f) {
		return;
	}

	vgui::surface()->DrawSetTextColor(FadeColour(teamColor, fadeTextMultiplier));

	if (settings->bShowHealth) {
		int healthMode = cl_neo_hud_health_mode.GetInt();
		V_snprintf(textASCII, MAX_MARKER_STRSIZE, healthMode ? "%dhp" : "%d%%", player->GetDisplayedHealth(healthMode));
		DisplayText(textASCII, y, MAX_MARKER_STRSIZE);
		y -= textSize;
	}

	if (settings->bShowHealthBar) {
		static constexpr int HEALTHBAR_WIDTH = 64;
		static constexpr int HEALTHBAR_HEIGHT = 6;

		int healthBarWidth = Ceil2Int(HEALTHBAR_WIDTH * Min((float)player->GetHealth() / player->GetMaxHealth(), 1.0f)); // Clamp in case someone forgot to set max health, or it hasn't been sent by the server yet.
		int healthBarYPos = y - HEALTHBAR_HEIGHT;
		vgui::surface()->DrawSetColor(FadeColour(COLOR_WHITE, fadeTextMultiplier * 0.8));
		vgui::surface()->DrawFilledRect(x - HEALTHBAR_WIDTH / 2, healthBarYPos, x - HEALTHBAR_WIDTH / 2 + healthBarWidth, healthBarYPos + HEALTHBAR_HEIGHT);
		vgui::surface()->DrawSetColor(FadeColour(COLOR_BLACK, fadeTextMultiplier * 0.8));
		vgui::surface()->DrawOutlinedRect(x - HEALTHBAR_WIDTH / 2, healthBarYPos, x + HEALTHBAR_WIDTH / 2, healthBarYPos + HEALTHBAR_HEIGHT);
				
		vgui::surface()->DrawSetColor(FadeColour(COLOR_BLACK, fadeTextMultiplier * 0.8));
		const float numChunks = (player->GetMaxHealth() / 25.f);
		const int chunkWidth = (HEALTHBAR_WIDTH) / numChunks;
		constexpr int HEALTHBAR_MIN_CHUNK_WIDTH = 2;
		if (chunkWidth >= HEALTHBAR_MIN_CHUNK_WIDTH)
		{
			for (int i = 1; i < numChunks; i++) {
				if (healthBarWidth <= i * chunkWidth) {
					break;
				}
				vgui::surface()->DrawLine((x - (HEALTHBAR_WIDTH / 2)) + (i * chunkWidth), healthBarYPos, (x - (HEALTHBAR_WIDTH / 2)) + (i * chunkWidth), healthBarYPos + HEALTHBAR_HEIGHT - 1);
			}
		}
		y -= HEALTHBAR_HEIGHT;
	}

	vgui::surface()->DrawSetTextColor(FadeColour(teamColor, fadeTextMultiplier));

	if (settings->bShowName) {
		auto playerName = player->GetPlayerNameWithTakeoverContext(player->entindex());
		const char *playerClantag = player->GetNeoClantag();

		if (settings->bPrependClantagToName && playerClantag && playerClantag[0])
		{
			V_sprintf_safe(textASCII, "[%s] %s", playerClantag, playerName);
		}
		else
		{
			V_strcpy_safe(textASCII, playerName);
		}
		DisplayText(textASCII, y, settings->iMaxNameLength);
	}
}

Color CNEOHud_FriendlyMarker::GetTeamColour(int team)
{
	return (team == TEAM_NSF) ? COLOR_NSF : COLOR_JINRAI;
}

union NeoIFFMarkerVariant
{
	int iVal;
	float flVal;
	bool bVal;
};

enum NeoIFFMarkerVariantType
{
	NEOIFFMARKERVARTYPE_INT = 0,
	NEOIFFMARKERVARTYPE_FLOAT,
	NEOIFFMARKERVARTYPE_BOOL,
};

static constexpr const NeoIFFMarkerVariantType NEOIFFMARKER_SEGMENT_VARTYPES[NEOIFFMARKER_SEGMENT__TOTAL] = {
	NEOIFFMARKERVARTYPE_INT,   // NEOIFFMARKER_SEGMENT_I_INITIALOFFSET
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_SHOWDISTANCE
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_VERBOSEDISTANCE
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_SHOWSQUADMARKER
	NEOIFFMARKERVARTYPE_FLOAT,   // NEOIFFMARKER_SEGMENT_FL_SQUADMARKERSCALE
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_SHOWHEALTHBAR
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_SHOWHEALTH
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_SHOWNAME
	NEOIFFMARKERVARTYPE_BOOL,   // NEOIFFMARKER_SEGMENT_B_PREPENDCLANTAGTONAME
	NEOIFFMARKERVARTYPE_INT,   // NEOIFFMARKER_SEGMENT_I_MAXNAMELENGTH
};

bool ImportMarker(FriendlyMarkerInfo *crh, const char *pszSequence)
{
	int iPrevSegment = 0;
	int iSegmentIdx = 0;
	NeoIFFMarkerVariant vars[NEOIFFMARKER_SEGMENT__TOTAL] = {};

	const int iPszSequenceLength = V_strlen(pszSequence);
	if (iPszSequenceLength <= 0 || iPszSequenceLength > NEO_IFFMARKER_SEQMAX)
	{
		return false;
	}

	char szMutSequence[NEO_IFFMARKER_SEQMAX];
	V_memcpy(szMutSequence, pszSequence, sizeof(char) * iPszSequenceLength);
	for (int i = 0; i < iPszSequenceLength && iSegmentIdx < NEOIFFMARKER_SEGMENT__TOTAL; ++i)
	{
		const char ch = szMutSequence[i];
		if (ch == ';')
		{
			szMutSequence[i] = '\0';
			const char *pszCurSegment = szMutSequence + iPrevSegment;
			const int iPszCurSegmentSize = i - iPrevSegment;
			if (iPszCurSegmentSize > 0)
			{
				switch (NEOIFFMARKER_SEGMENT_VARTYPES[iSegmentIdx])
				{
				case NEOIFFMARKERVARTYPE_INT:
					vars[iSegmentIdx].iVal = atoi(pszCurSegment);
					break;
				case NEOIFFMARKERVARTYPE_FLOAT:
					vars[iSegmentIdx].flVal = atof(pszCurSegment);
					break;
				case NEOIFFMARKERVARTYPE_BOOL:
					vars[iSegmentIdx].bVal = (atoi(pszCurSegment) != 0);
					break;
				}
			}
			iPrevSegment = i + 1;
			++iSegmentIdx;
		}
	}

	if (iSegmentIdx != NEOIFFMARKER_SEGMENT__TOTAL)
	{
		return false;
	}

	crh->iInitialOffset = vars[NEOIFFMARKER_SEGMENT_I_INITIALOFFSET].iVal;
	crh->bShowDistance = vars[NEOIFFMARKER_SEGMENT_B_SHOWDISTANCE].bVal;
	crh->bVerboseDistance = vars[NEOIFFMARKER_SEGMENT_B_VERBOSEDISTANCE].bVal;
	crh->bShowSquadMarker = vars[NEOIFFMARKER_SEGMENT_B_SHOWSQUADMARKER].bVal;
	crh->flSquadMarkerScale = vars[NEOIFFMARKER_SEGMENT_FL_SQUADMARKERSCALE].flVal;
	crh->bShowHealthBar = vars[NEOIFFMARKER_SEGMENT_B_SHOWHEALTHBAR].bVal;
	crh->bShowHealth = vars[NEOIFFMARKER_SEGMENT_B_SHOWHEALTH].bVal;
	crh->bShowName = vars[NEOIFFMARKER_SEGMENT_B_SHOWNAME].bVal;
	crh->bPrependClantagToName = vars[NEOIFFMARKER_SEGMENT_B_PREPENDCLANTAGTONAME].bVal;
	crh->iMaxNameLength = vars[NEOIFFMARKER_SEGMENT_I_MAXNAMELENGTH].iVal;

	return true;
}

void ExportMarker(const FriendlyMarkerInfo *crh, char (&szSequence)[NEO_IFFMARKER_SEQMAX])
{
	V_sprintf_safe(szSequence,
			"%d;%d;%d;%d;%.2f;%d;%d;%d;%d;%d;",
			crh->iInitialOffset,
			static_cast<int>(crh->bShowDistance),
			static_cast<int>(crh->bVerboseDistance),
			static_cast<int>(crh->bShowSquadMarker),
			crh->flSquadMarkerScale,
			static_cast<int>(crh->bShowHealthBar),
			static_cast<int>(crh->bShowHealth),
			static_cast<int>(crh->bShowName),
			static_cast<int>(crh->bPrependClantagToName),
			crh->iMaxNameLength);
}
