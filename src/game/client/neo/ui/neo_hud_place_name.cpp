#include "neo_hud_place_name.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>
#include "c_neo_player.h"
#include "view.h"
#include "tier1/lzmaDecoder.h"
#include "smoke_fog_overlay.h"
#include "nav_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_PlaceName, neo_place_name);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(PlaceName, 0.1)

static CNEOHud_PlaceName *g_PlaceName = nullptr;

static const char* TEXT_MATERIAL = "vgui/callout_text";

CNEOHud_PlaceName::CNEOHud_PlaceName(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_PLACE_NAME;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}
	
	SetVisible(true);
	
	g_PlaceName = this;
    m_szPlaceName[0] = L'\0';

	PrecacheMaterial( TEXT_MATERIAL );
	m_Font.Init(TEXT_MATERIAL, TEXTURE_GROUP_PRECACHED, true);
}

CNEOHud_PlaceName::~CNEOHud_PlaceName()
{
	if (g_PlaceName == this)
	{
		g_PlaceName = nullptr;
	}
}

void CNEOHud_PlaceName::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide = 0, tall = 0;
	vgui::surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

enum
{
	TEXTALIGN_LEFT = 0,
	TEXTALIGN_CENTER,
	TEXTALIGN_RIGHT
};
void CNEOHud_PlaceName::UpdateStateForNeoHudElementDraw()
{
	C_NEO_Player* pTargetPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pTargetPlayer)
	{
		return;
	}

	if (pTargetPlayer->IsPlayerDead())
	{
		if (const int observerMode = pTargetPlayer->GetObserverMode();
			observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
		{
			if (C_BaseEntity* pObserverTarget = pTargetPlayer->GetObserverTarget();
				pObserverTarget && pObserverTarget->IsPlayer())
			{
				pTargetPlayer = static_cast<C_NEO_Player*>(pObserverTarget);
			}
		}
	}

	V_snwprintf(m_szPlaceName, MAX_PLACE_NAME_LENGTH, L"%hs", pTargetPlayer->GetLastKnownPlaceName());
	switch (textXAlignment)
	{
		case TEXTALIGN_LEFT:
		default:
			textXOffset = 0;
			break;
		case TEXTALIGN_CENTER:
		case TEXTALIGN_RIGHT:
			int textWidth = 0, textHeight = 0;
			vgui::surface()->GetTextSize(textFont, m_szPlaceName, textWidth, textHeight);
			textXOffset = textXAlignment == TEXTALIGN_CENTER ? (wide / 2) - (textWidth / 2) : wide - textWidth;
			break;
	}
}

ConVar cl_neo_hud_current_place_name_draw("cl_neo_hud_current_place_name_draw", "1", FCVAR_ARCHIVE, "Draw the current place name", true, 0.0f, true, 1.0f);

void CNEOHud_PlaceName::DrawNeoHudElement()
{
	if (!ShouldDraw())
		return;

	if (cl_neo_hud_current_place_name_draw.GetBool())
	{
		vgui::surface()->DrawSetTextFont(textFont);
		vgui::surface()->DrawSetTextColor(textColor);
		vgui::surface()->DrawSetTextPos(textXOffset, 0);
		vgui::surface()->DrawPrintText(m_szPlaceName, V_wcslen(m_szPlaceName));
	}
}

void CNEOHud_PlaceName::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

static bool shouldDrawPlaceNames = false;
static ConCommand startshowplacenames("+showPlaceNames", [](const CCommand& args)->void {shouldDrawPlaceNames = true; });
static ConCommand endshowplacenames("-showPlaceNames", [](const CCommand& args)->void {shouldDrawPlaceNames = false; });

static float placeNameRadiusSquared = 0.0f;
ConVar cl_neo_hud_place_name_radius("cl_neo_hud_place_name_radius", "1024", FCVAR_ARCHIVE, "Radius from the camera within which to draw the names of all nearby places", true, 0.0f, false, 0.0f,
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		placeNameRadiusSquared = pow(cl_neo_hud_place_name_radius.GetFloat(), 2);
});

static float animationAlpha = 0.f;
void CNEOHud_PlaceName::DrawPlaceNames()
{
	constexpr int ANIMATION_SPEED = 2;
	animationAlpha =   shouldDrawPlaceNames	? min(1.0f, animationAlpha + (gpGlobals->frametime * ANIMATION_SPEED))
											: max(0.0f, animationAlpha - (gpGlobals->frametime * ANIMATION_SPEED));

	for (PlaceNameCallout place : places)
	{
		if (place.navAreaCount <= 0)
		{
			continue;
		}

		float alpha = animationAlpha;
		if (placeNameRadiusSquared != 0)
		{
			if (const float distanceSquared = MainViewOrigin().DistToSqr(place.pointWorldText.GetAbsOrigin());
				distanceSquared > placeNameRadiusSquared)
			{
				alpha *= 1.f - min(1.f, ((distanceSquared - placeNameRadiusSquared) / placeNameRadiusSquared));
			}
		}

		alpha -= g_SmokeFogOverlayAlpha;
		alpha = clamp(alpha, 0.f, 1.f);

		place.pointWorldText.SetAlpha(255 * alpha);
		place.pointWorldText.DrawModel();
	}
}

ConVar cl_neo_hud_place_name_text_size("cl_neo_hud_place_name_text_size", "32", FCVAR_ARCHIVE, "Place name text size", true, 1.f, false, 0.f, 
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		if (g_PlaceName)
		{
			g_PlaceName->SetPlaceNameTextSize(cl_neo_hud_place_name_text_size.GetFloat());
		}

});
void CNEOHud_PlaceName::SetPlaceNameTextSize(const float textSize)
{
	for (int i=0; i<places.Count(); i++)
	{
		places[i].pointWorldText.SetTextSize(textSize);
	}
}

ConVar cl_neo_hud_place_name_text_spacing_x("cl_neo_hud_place_name_text_spacing_x", "-8", FCVAR_ARCHIVE, "Place name spacing between characters", false, 0.f, false, 0.f, 
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		if (g_PlaceName)
		{
			g_PlaceName->SetPlaceNameTextSpacingX(cl_neo_hud_place_name_text_spacing_x.GetFloat());
		}

});
void CNEOHud_PlaceName::SetPlaceNameTextSpacingX(const float textSpacingX)
{
	for (int i=0; i<places.Count(); i++)
	{
		places[i].pointWorldText.SetTextSpacingX(textSpacingX);
	}
}

ConVar cl_neo_hud_place_name_offset("cl_neo_hud_place_name_offset", "96", FCVAR_ARCHIVE, "Place name offset from place origin", false, 0.f, false, 0.f, 
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		if (g_PlaceName)
		{
			g_PlaceName->SetPlaceNameOffset(cl_neo_hud_place_name_offset.GetFloat());
		}

});
void CNEOHud_PlaceName::SetPlaceNameOffset(const float offset)
{
	const Vector vOffset = Vector(0, 0, offset);
	for (int i=0; i<places.Count(); i++)
	{
		places[i].pointWorldText.SetAbsOrigin(places[i].origin + vOffset);
	}
}


ConVar cl_neo_hud_place_name_orientation("cl_neo_hud_place_name_orientation", "3", FCVAR_ARCHIVE, "Place name orientation", true, 0, true, POINTWORLDTEXTORIENTATION__TOTAL - 1, 
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		if (g_PlaceName)
		{
			g_PlaceName->SetPlaceNameOrientation((PointWorldTextOrientation)cl_neo_hud_place_name_orientation.GetInt());
		}

});
void CNEOHud_PlaceName::SetPlaceNameOrientation(const PointWorldTextOrientation orientation)
{
	for (int i=0; i<places.Count(); i++)
	{
		places[i].pointWorldText.SetOrientation(orientation);
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Fetch raw nav data into buffer
 */
NavErrorType CNEOHud_PlaceName::GetNavDataFromFile( CUtlBuffer &outBuffer, bool *pNavDataFromBSP )
{
	char maptmp[256];
	Q_FileBase( engine->GetLevelName(), maptmp, sizeof( maptmp) );
	const char* pszMapName = maptmp;

	// nav filename is derived from map filename
	char filename[MAX_PATH] = { 0 };
	Q_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, pszMapName );

	if ( !filesystem->ReadFile( filename, "MOD", outBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		if ( !filesystem->ReadFile( filename, "BSP", outBuffer ) )	// ... and this looks for one if it's the only one around.
		{
			// Finally, check for the special embed name for in-BSP nav meshes only
			if ( !filesystem->ReadFile( PATH_NAVFILE_EMBEDDED, "BSP", outBuffer ) )
			{
				return NAV_CANT_ACCESS_FILE;
			}
		}
		if ( pNavDataFromBSP )
		{
			*pNavDataFromBSP = true;
		}
	}

	if ( IsX360() )
	{
		// 360 has compressed NAVs
		if ( CLZMA::IsCompressed( (unsigned char *)outBuffer.Base() ) )
		{
			int originalSize = CLZMA::GetActualSize( (unsigned char *)outBuffer.Base() );
			unsigned char *pOriginalData = new unsigned char[originalSize];
			CLZMA::Uncompress( (unsigned char *)outBuffer.Base(), pOriginalData );
			outBuffer.AssumeMemory( pOriginalData, originalSize, originalSize, CUtlBuffer::READ_ONLY );
		}
	}

	return NAV_OK;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Reads the used place names from the nav file (can be used to selectively precache before the nav is loaded)
 */
void CNEOHud_PlaceName::GetPlacesFromNavFile()
{
	places.RemoveAll();
	// nav filename is derived from map filename
	char filename[256];
	Q_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( engine->GetLevelName() ) );

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( GetNavDataFromFile( fileBuffer ) != NAV_OK )
	{
		return;
	}

	// check magic number
	unsigned int magic = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || magic != NAV_MAGIC_NUMBER )
	{
		return;	// Corrupt nav file?
	}

	// read file version number
	unsigned int version = fileBuffer.GetUnsignedInt();
	if ( !fileBuffer.IsValid() || version > NavCurrentVersion )
	{
		return;	// Unknown nav file version
	}

	if ( version < 5 )
	{
		return;	// Too old to have place names
	}

	unsigned int subVersion = 0;
	if ( version >= 10 )
	{
		subVersion = fileBuffer.GetUnsignedInt();
		if ( !fileBuffer.IsValid() )
		{
			return;	// No sub-version
		}
	}

	fileBuffer.GetUnsignedInt();	// skip BSP file size (version >= 4)
	if ( version >= 14 )
	{
		fileBuffer.GetUnsignedChar();	// skip m_isAnalyzed
	}

	{
		// read number of entries
		unsigned short placeCount = fileBuffer.GetUnsignedShort();

		places.RemoveAll();

		// read each entry
		char placeName[256];
		unsigned short len;
		for( int i=0; i<placeCount; ++i )
		{
			len = fileBuffer.GetUnsignedShort();
			fileBuffer.Get( placeName, MIN( sizeof( placeName ), len ) );
			int index = places.AddToTail({ {placeName, {0, 0, 0}, &m_Font}, {0, 0, 0}, 0 });
			places[index].pointWorldText.SetTextSize(cl_neo_hud_place_name_text_size.GetFloat());
			places[index].pointWorldText.SetTextSpacingX(cl_neo_hud_place_name_text_spacing_x.GetFloat());
			places[index].pointWorldText.SetOrientation((PointWorldTextOrientation)cl_neo_hud_place_name_orientation.GetInt());
		}

		if (version > 11)
		{
			fileBuffer.GetUnsignedChar(); // Skip has unnamed areas
		}
	}

	// get number of areas
	unsigned int areaCount = fileBuffer.GetUnsignedInt();
	unsigned int i;

	if (areaCount == 0)
	{
		return;
	}

	// Read each nav area
	for (i = 0; i < areaCount; ++i)
	{
		fileBuffer.GetUnsignedInt(); // Skip ID

		if (version <= 8) // Skip attribute flags
		{
			fileBuffer.GetUnsignedChar();
		}
		else if (version < 13)
		{
			fileBuffer.GetUnsignedShort();
		}
		else
		{
			fileBuffer.GetUnsignedInt();
		}

		Vector nwCorner;
		Vector seCorner;
		fileBuffer.Get(&nwCorner, 3 * sizeof(float));
		fileBuffer.Get(&seCorner, 3 * sizeof(float));

		fileBuffer.GetFloat(); // Skip heights of implicit corners
		fileBuffer.GetFloat();

		for (int d = 0; d < NavDirType::NUM_DIRECTIONS; d++)
		{
			unsigned int connectionCount = fileBuffer.GetUnsignedInt();
			Assert(fileBuffer.IsValid());

			for (unsigned int j = 0; j < connectionCount; ++j)
			{
				fileBuffer.GetUnsignedInt(); // Skip connection ID
				Assert(fileBuffer.IsValid());
			}
		}

		unsigned char hidingSpotCount = fileBuffer.GetUnsignedChar();
		for (unsigned char h = 0; h < hidingSpotCount; ++h)
		{
			fileBuffer.GetUnsignedInt(); // Skip hiding spot ID
			fileBuffer.GetFloat(); // Skip hiding spot pos X
			fileBuffer.GetFloat(); // Skip hiding spot pos Y
			fileBuffer.GetFloat(); // Skip hiding spot pos Z
			fileBuffer.GetUnsignedChar(); // Skip hiding spot flags
		}

		if (version < 15)
		{
			// Skip the approach areas
			unsigned char nToEat = fileBuffer.GetUnsignedChar();
			for (unsigned char a = 0; a < nToEat; ++a)
			{
				fileBuffer.GetUnsignedInt();
				fileBuffer.GetUnsignedInt();
				fileBuffer.GetUnsignedChar();
				fileBuffer.GetUnsignedInt();
				fileBuffer.GetUnsignedChar();
			}
		}

		// Skip encounter paths
		unsigned int encounterCount = fileBuffer.GetUnsignedInt();
		for (unsigned int e = 0; e < encounterCount; ++e)
		{
			fileBuffer.GetUnsignedInt(); // Skip from ID
			fileBuffer.GetUnsignedChar(); // Skip from dir
			fileBuffer.GetUnsignedInt(); // Skip to ID
			fileBuffer.GetUnsignedChar(); // Skip to dir

			unsigned char spotCount = fileBuffer.GetUnsignedChar();
			for(unsigned char s=0; s<spotCount; ++s)
			{
				fileBuffer.GetUnsignedInt(); // Skip spot ID
				fileBuffer.GetUnsignedChar(); // Skip parametric distance along ray...
			}
		}

		// Place data
		unsigned short entry = fileBuffer.GetUnsignedShort();
		if (entry > 0 && entry <= places.Count())
		{
			entry -= 1;
			Vector newNavCenter = (nwCorner + seCorner) / 2.f;
			places[entry].origin = ((places[entry].origin * places[entry].navAreaCount) + newNavCenter) / ++places[entry].navAreaCount;
			places[entry].pointWorldText.SetAbsOrigin(places[entry].origin + Vector(0, 0, cl_neo_hud_place_name_offset.GetFloat()));
		}

		if (version < 7)
		{
			continue;
		}

		// Skip ladder data
		for (int dir=0; dir<LadderDirectionType::NUM_LADDER_DIRECTIONS; ++dir)
		{
			unsigned int ladderConnectionCount = fileBuffer.GetUnsignedInt();
			for (unsigned int j = 0; j < ladderConnectionCount; ++j)
			{
				fileBuffer.GetUnsignedInt(); // Skip ladder connection ID
			}
		}

		if (version < 8)
		{
			continue;
		}

		// Skip earliest occupy times
		for (int j = 0; j < MAX_NAV_TEAMS; ++j)
		{
			fileBuffer.GetFloat();
		}

		if (version < 11)
		{
			continue;
		}

		// Skip light intensity
		for (int j = 0; j <NavCornerType::NUM_CORNERS; ++j)
		{
			fileBuffer.GetFloat();
		}

		if (version < 16)
		{
			continue;
		}
		
		// Skip visibility information
		unsigned int visibleAreaCount = fileBuffer.GetUnsignedInt();
		for (unsigned int j = 0; j < visibleAreaCount; ++j)
		{
			fileBuffer.GetUnsignedInt(); // Skip area ID
			fileBuffer.GetUnsignedChar(); // Skip area attributes
		}

		fileBuffer.GetUnsignedInt(); // Skip inherited visibility from ID
	}

	return;
}

CNEOHud_PlaceName* GetPlaceName()
{
	return g_PlaceName;
}
