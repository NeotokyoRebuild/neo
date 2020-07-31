#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "team.h"
#include "viewport_panel_names.h"
#include "neo_model_manager.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say( edict_t *pEdict, bool teamonly );

ConVar sv_motd_unload_on_dismissal( "sv_motd_unload_on_dismissal",
	"0", 0,
	"If enabled, the MOTD contents will be unloaded when the player \
closes the MOTD." );

extern CBaseEntity*	FindPickerEntityClass( CBasePlayer *pPlayer,
	char *classname );
extern bool			g_fGameOver;

void FinishClientPutInServer( CNEO_Player *pPlayer )
{
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// This causes problems with Source engine, purge!
		const int bellCharCode = 7;

		// Replace it with a space
		if ( *pApersand == '%' || *pApersand == bellCharCode)
				*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );

	if ( NEORules()->IsTeamplay() == true )
	{
		ClientPrint( pPlayer, HUD_PRINTTALK, "You are on team %s1\n", pPlayer->GetTeam()->GetName() );
	}

	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	KeyValues *data = new KeyValues("data");
	data->SetString( "title", title );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	"motd" );		// use this stringtable entry
	data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );

	pPlayer->ShowViewPortPanel( PANEL_INFO, true, data );

	data->deleteThis();

	// NEO TODO (Rain): Team selection HUD here.

	// If player chooses to spectate upon joining, start in free roam mode.
	pPlayer->StartObserverMode(OBS_MODE_ROAMING);
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CNEO_Player *pPlayer = CNEO_Player::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in NEO multiplayer!
	Assert( !bLoadGame );

	CNEO_Player *pPlayer = ToNEOPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return NEO_GAME_NAME;
}

//-----------------------------------------------------------------------------
// Purpose: Given a player and optional name returns the entity of that 
//			classname that the player is nearest facing
//			
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity* FindEntity( edict_t *pEdict, char *classname)
{
	// If no name was given set bits based on the picked
	if (FStrEq(classname,"")) 
	{
		return (FindPickerEntityClass( static_cast<CBasePlayer*>(GetContainingEntity(pEdict)), classname ));
	}
	return NULL;
}

// These are what hl2mp_client.cpp calls on ClientGamePrecache().
// NEO TODO (Rain): we can probably skip this once we're guaranteed
// to no longer use any of these.
void Precache_HL2MP( void )
{
	CBaseEntity::PrecacheModel("models/player.mdl");
	CBaseEntity::PrecacheModel("models/gibs/agibs.mdl");
	CBaseEntity::PrecacheModel("models/weapons/v_hands.mdl");

	CBaseEntity::PrecacheScriptSound( "HUDQuickInfo.LowAmmo" );
	CBaseEntity::PrecacheScriptSound( "HUDQuickInfo.LowHealth" );

	CBaseEntity::PrecacheScriptSound( "FX_AntlionImpact.ShellImpact" );
	CBaseEntity::PrecacheScriptSound( "Missile.ShotDown" );
	CBaseEntity::PrecacheScriptSound( "Bullets.DefaultNearmiss" );
	CBaseEntity::PrecacheScriptSound( "Bullets.GunshipNearmiss" );
	CBaseEntity::PrecacheScriptSound( "Bullets.StriderNearmiss" );
	
	CBaseEntity::PrecacheScriptSound( "Geiger.BeepHigh" );
	CBaseEntity::PrecacheScriptSound( "Geiger.BeepLow" );
}

void Precache_NEO_Sounds( void )
{
	CBaseEntity::PrecacheScriptSound("weapon_milso.reload");
	CBaseEntity::PrecacheScriptSound("weapon_milso.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_milso.empty");
	CBaseEntity::PrecacheScriptSound("weapon_milso.single");
	CBaseEntity::PrecacheScriptSound("weapon_milso.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_tachi.reload");
	CBaseEntity::PrecacheScriptSound("weapon_tachi.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_tachi.empty");
	CBaseEntity::PrecacheScriptSound("weapon_tachi.single");
	CBaseEntity::PrecacheScriptSound("weapon_tachi.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_kyla.reload");
	CBaseEntity::PrecacheScriptSound("weapon_kyla.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_kyla.empty");
	CBaseEntity::PrecacheScriptSound("weapon_kyla.single");
	CBaseEntity::PrecacheScriptSound("weapon_kyla.NPC_Single");

	CBaseEntity::PrecacheScriptSound("weapon_aa13.reload");
	CBaseEntity::PrecacheScriptSound("weapon_aa13.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_aa13.empty");
	CBaseEntity::PrecacheScriptSound("weapon_aa13.single");
	CBaseEntity::PrecacheScriptSound("weapon_aa13.NPC_Single");

	CBaseEntity::PrecacheScriptSound("weapon_murata.load");
	CBaseEntity::PrecacheScriptSound("weapon_murata.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_murata.pump");
	CBaseEntity::PrecacheScriptSound("weapon_murata.npc_pump");
	CBaseEntity::PrecacheScriptSound("weapon_murata.empty");
	CBaseEntity::PrecacheScriptSound("weapon_murata.single");
	CBaseEntity::PrecacheScriptSound("weapon_murata.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_mpn45.reload");
	CBaseEntity::PrecacheScriptSound("weapon_mpn45.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_mpn45.empty");
	CBaseEntity::PrecacheScriptSound("weapon_mpn45.single");
	CBaseEntity::PrecacheScriptSound("weapon_mpn45.NPC_Single");

	CBaseEntity::PrecacheScriptSound("weapon_mpn45s.single");
	CBaseEntity::PrecacheScriptSound("weapon_mpn45s.NPC_Single");

	CBaseEntity::PrecacheScriptSound("weapon_srm.reload");
	CBaseEntity::PrecacheScriptSound("weapon_srm.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_srm.empty");
	CBaseEntity::PrecacheScriptSound("weapon_srm.single");
	CBaseEntity::PrecacheScriptSound("weapon_srm.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_srms.single");
	CBaseEntity::PrecacheScriptSound("weapon_srms.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_jitte.reload");
	CBaseEntity::PrecacheScriptSound("weapon_jitte.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_jitte.empty");
	CBaseEntity::PrecacheScriptSound("weapon_jitte.single");
	CBaseEntity::PrecacheScriptSound("weapon_jitte.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_jitte_scoped.single");
	CBaseEntity::PrecacheScriptSound("weapon_jitte_scoped.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_m41.reload");
	CBaseEntity::PrecacheScriptSound("weapon_m41.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_m41.empty");
	CBaseEntity::PrecacheScriptSound("weapon_m41.single");
	CBaseEntity::PrecacheScriptSound("weapon_m41.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_m41l.single");
	CBaseEntity::PrecacheScriptSound("weapon_m41l.npc_single");
	CBaseEntity::PrecacheScriptSound("weapon_m41s.single");
	CBaseEntity::PrecacheScriptSound("weapon_m41s.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_srs.reload");
	CBaseEntity::PrecacheScriptSound("weapon_srs.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_srs.charge");
	CBaseEntity::PrecacheScriptSound("weapon_srs.npc_charge");
	CBaseEntity::PrecacheScriptSound("weapon_srs.empty");
	CBaseEntity::PrecacheScriptSound("weapon_srs.single");
	CBaseEntity::PrecacheScriptSound("weapon_srs.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_zr68.reload");
	CBaseEntity::PrecacheScriptSound("weapon_zr68.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_zr68.empty");
	CBaseEntity::PrecacheScriptSound("weapon_zr68.single");
	CBaseEntity::PrecacheScriptSound("weapon_zr68.npc_single");

	CBaseEntity::PrecacheScriptSound("weapon_zr68l.single");
	CBaseEntity::PrecacheScriptSound("weapon_zr68l.npc_single");
	CBaseEntity::PrecacheScriptSound("weapon_zr68l.reload");
	CBaseEntity::PrecacheScriptSound("weapon_zr68l.npc_reload");

	CBaseEntity::PrecacheScriptSound("weapon_zr68s.fire");
	CBaseEntity::PrecacheScriptSound("weapon_zr68s.npc_fire");

	CBaseEntity::PrecacheScriptSound("weapon_mx.reload");
	CBaseEntity::PrecacheScriptSound("weapon_mx.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_mx.empty");
	CBaseEntity::PrecacheScriptSound("weapon_mX.fire");
	CBaseEntity::PrecacheScriptSound("weapon_mX.npc_fire");

	CBaseEntity::PrecacheScriptSound("weapon_mXs.fire");
	CBaseEntity::PrecacheScriptSound("weapon_mXs.npc_fire");

	CBaseEntity::PrecacheScriptSound("weapon_pz.reload");
	CBaseEntity::PrecacheScriptSound("weapon_pz.npc_reload");
	CBaseEntity::PrecacheScriptSound("weapon_pz.empty");
	CBaseEntity::PrecacheScriptSound("weapon_pz.single");
	CBaseEntity::PrecacheScriptSound("weapon_pz.npc_single");

	// NEO TODO (Rain):
	// These commented three are set as "pz_empty.wav" in NT sound scripts,
	// probably hardcoded in the original game. We should probably
	// set up our own sound script file for these and any other custom sounds
	// we might want to use (sound effect for setting Tachi firemode comes to mind).
	//
	//CBaseEntity::PrecacheScriptSound("weapon_remotedet.reload");
	//CBaseEntity::PrecacheScriptSound("weapon_remotedet.npc_reload");
	//CBaseEntity::PrecacheScriptSound("weapon_remotedet.empty");
	CBaseEntity::PrecacheScriptSound("weapon_remotedet.single");
	CBaseEntity::PrecacheScriptSound("weapon_remotedet.npc_single");

	CBaseEntity::PrecacheScriptSound("Smoke.Explode");
	CBaseEntity::PrecacheScriptSound("Default.PullPin_Grenade");
	CBaseEntity::PrecacheScriptSound("Grenade.Bounce");
	CBaseEntity::PrecacheScriptSound("BaseGrenade.Explode");
	CBaseEntity::PrecacheScriptSound("BaseExplosionEffect.Sound");

	CBaseEntity::PrecacheScriptSound("Weapon_Generic.melee_swing");

	CBaseEntity::PrecacheScriptSound("HUD.GhostEquip");
	CBaseEntity::PrecacheScriptSound("HUD.GhostPickUp");
	CBaseEntity::PrecacheScriptSound("HUD.JinraiWin");
	CBaseEntity::PrecacheScriptSound("HUD.NSFWin");
	CBaseEntity::PrecacheScriptSound("HUD.Draw");
	CBaseEntity::PrecacheScriptSound("NeoPlayer.GhostPing");
	CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOn");
	CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOff");
	CBaseEntity::PrecacheScriptSound("NeoPlayer.VisionOn");
	CBaseEntity::PrecacheScriptSound("Victory.Draw");
	CBaseEntity::PrecacheScriptSound("Victory.Jinrai");
	CBaseEntity::PrecacheScriptSound("Victory.NSF");
}

//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	Precache_HL2MP();
	
	CNEOModelManager *modelManager = CNEOModelManager::Instance();
	if (!modelManager)
	{
		Error("Failed to get CNEOModelManager instance\n");
	}
	modelManager->Precache();

	Precache_NEO_Sounds();
}

// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	CNEO_Player *pPlayer = ToNEOPlayer( pEdict );

	if ( pPlayer )
	{
		if ( gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME )
		{
			if (NEORules()->FPlayerCanRespawn(pPlayer))
			{
				// respawn player
				pPlayer->Spawn();
			}
		}
		else
		{
			pPlayer->SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}
}

ConVar sv_neo_bot_think("sv_neo_bot_think",
#ifdef DEBUG
	"1",
#else
	"0",
#endif
	FCVAR_NONE, "Run think on debug bots.", true, 0.0, true, 1.0);

void GameStartFrame( void )
{
	VPROF("GameStartFrame()");
	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = (teamplay.GetInt() != 0);

	if (sv_neo_bot_think.GetBool())
	{
		extern void Bot_RunAll();
		Bot_RunAll();
	}
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	CreateGameRulesObject( "CNEORules" );
}