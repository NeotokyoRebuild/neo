#include "c_neo_point_world_text.h"
#include "view_scene.h"
#include "view.h"

typedef struct Character {
  int codePoint, x, y, width, height, originX, originY, advance;
} Character;

typedef struct Font {
  const char *name;
  int size, bold, italic, width, height, characterCount;
  Character *characters;
} Font;

static Character characters_Roboto_Mono[] = {
  {' ', 167, 352, 12, 12, 6, 6, 77},
  {'!', 675, 144, 27, 104, -24, 97, 77},
  {'"', 1859, 249, 44, 42, -16, 102, 77},
  {'#', 702, 144, 82, 103, 2, 97, 77},
  {'$', 324, 0, 70, 131, -4, 112, 77},
  {'%', 1322, 0, 83, 106, 3, 98, 77},
  {'&', 1405, 0, 80, 106, -1, 98, 77},
  {'\'', 1903, 249, 22, 42, -25, 102, 77},
  {'(', 0, 0, 45, 144, -16, 109, 77},
  {')', 45, 0, 45, 144, -14, 109, 77},
  {'*', 1497, 249, 72, 73, -4, 97, 77},
  {'+', 1422, 249, 75, 78, -1, 81, 77},
  {',', 1829, 249, 30, 47, -16, 20, 77},
  {'-', 37, 352, 60, 22, -8, 51, 77},
  {'.', 2008, 249, 30, 30, -25, 22, 77},
  {'/', 568, 0, 60, 111, -10, 97, 77},
  {'0', 1788, 0, 71, 106, -3, 98, 77},
  {'1', 342, 249, 47, 103, -7, 97, 77},
  {'2', 0, 144, 74, 105, 1, 98, 77},
  {'3', 1859, 0, 70, 106, 0, 98, 77},
  {'4', 1263, 144, 78, 103, 1, 97, 77},
  {'5', 572, 144, 69, 104, -6, 97, 77},
  {'6', 74, 144, 70, 105, -3, 97, 77},
  {'7', 1491, 144, 73, 103, -1, 97, 77},
  {'8', 1929, 0, 70, 106, -5, 98, 77},
  {'9', 144, 144, 70, 105, -3, 98, 77},
  {':', 459, 249, 30, 85, -28, 77, 77},
  {';', 641, 144, 34, 104, -24, 77, 77},
  {'<', 1636, 249, 65, 69, -5, 75, 77},
  {'=', 1761, 249, 68, 48, -5, 65, 77},
  {'>', 1569, 249, 67, 69, -5, 75, 77},
  {'?', 283, 144, 66, 105, -6, 98, 77},
  {'@', 349, 144, 80, 104, 2, 97, 77},
  {'A', 865, 144, 80, 103, 1, 97, 77},
  {'B', 1852, 144, 71, 103, -5, 97, 77},
  {'C', 1713, 0, 75, 106, -1, 98, 77},
  {'D', 1417, 144, 74, 103, -4, 97, 77},
  {'E', 71, 249, 68, 103, -5, 97, 77},
  {'F', 139, 249, 68, 103, -6, 97, 77},
  {'G', 1485, 0, 76, 106, 0, 98, 77},
  {'H', 1923, 144, 71, 103, -3, 97, 77},
  {'I', 275, 249, 67, 103, -5, 97, 77},
  {'J', 501, 144, 71, 104, 0, 97, 77},
  {'K', 1341, 144, 76, 103, -5, 97, 77},
  {'L', 207, 249, 68, 103, -6, 97, 77},
  {'M', 1564, 144, 72, 103, -3, 97, 77},
  {'N', 0, 249, 71, 103, -3, 97, 77},
  {'O', 1561, 0, 76, 106, -1, 98, 77},
  {'P', 1636, 144, 72, 103, -6, 97, 77},
  {'Q', 416, 0, 79, 120, 0, 98, 77},
  {'R', 1708, 144, 72, 103, -5, 97, 77},
  {'S', 1637, 0, 76, 106, -1, 98, 77},
  {'T', 945, 144, 80, 103, 1, 97, 77},
  {'U', 429, 144, 72, 104, -3, 97, 77},
  {'V', 1105, 144, 79, 103, 1, 97, 77},
  {'W', 784, 144, 81, 103, 1, 97, 77},
  {'X', 1184, 144, 79, 103, 0, 97, 77},
  {'Y', 1025, 144, 80, 103, 2, 97, 77},
  {'Z', 1780, 144, 72, 103, -1, 97, 77},
  {'[', 90, 0, 37, 136, -21, 110, 77},
  {'\\', 628, 0, 60, 111, -9, 97, 77},
  {']', 127, 0, 37, 136, -19, 110, 77},
  {'^', 1701, 249, 60, 61, -9, 97, 77},
  {'_', 97, 352, 70, 21, -4, 6, 77},
  {'`', 0, 352, 37, 29, -20, 99, 77},
  {'a', 706, 249, 70, 82, -4, 75, 77},
  {'b', 688, 0, 70, 109, -5, 102, 77},
  {'c', 635, 249, 71, 82, -3, 75, 77},
  {'d', 758, 0, 69, 109, -3, 102, 77},
  {'e', 563, 249, 72, 82, -2, 75, 77},
  {'f', 495, 0, 73, 111, -4, 105, 77},
  {'g', 898, 0, 69, 108, -3, 75, 77},
  {'h', 1036, 0, 68, 108, -5, 102, 77},
  {'i', 214, 144, 69, 105, -7, 98, 77},
  {'j', 272, 0, 52, 132, -7, 98, 77},
  {'k', 827, 0, 71, 108, -5, 102, 77},
  {'l', 967, 0, 69, 108, -7, 102, 77},
  {'m', 845, 249, 78, 81, 0, 75, 77},
  {'n', 923, 249, 68, 81, -5, 75, 77},
  {'o', 489, 249, 74, 82, -2, 75, 77},
  {'p', 1184, 0, 69, 107, -5, 75, 77},
  {'q', 1253, 0, 69, 107, -3, 75, 77},
  {'r', 1058, 249, 59, 81, -15, 75, 77},
  {'s', 776, 249, 69, 82, -5, 75, 77},
  {'t', 389, 249, 70, 97, -3, 90, 77},
  {'u', 991, 249, 67, 81, -5, 74, 77},
  {'v', 1200, 249, 76, 80, 0, 74, 77},
  {'w', 1117, 249, 83, 80, 3, 74, 77},
  {'x', 1276, 249, 76, 80, -1, 74, 77},
  {'y', 1104, 0, 80, 107, 2, 74, 77},
  {'z', 1352, 249, 70, 80, -4, 74, 77},
  {'{', 164, 0, 54, 135, -14, 106, 77},
  {'|', 394, 0, 22, 128, -28, 97, 77},
  {'}', 218, 0, 54, 135, -14, 106, 77},
  {'~', 1925, 249, 83, 37, 3, 56, 77},
};

static Font font_Roboto_Mono = {"Roboto Mono", 128, 0, 0, 2048, 512, 95, characters_Roboto_Mono};

PointWorldText::PointWorldText()
{
	V_memset(m_szText, 0, sizeof(m_szText));
}

PointWorldText::PointWorldText(char* pszText, Vector pos, CMaterialReference* font)
{
	m_vecAbsOrigin = pos;
	m_Font = font;
	SetText(pszText);
}

PointWorldText::~PointWorldText()
{
}

void PointWorldText::SetText( const char* pszText )
{
	m_nTextLength = V_strlen( pszText );
	V_strncpy( m_szText, pszText, sizeof(m_szText) );
	UpdateTextWorldSize();
}

void PointWorldText::UpdateTextWorldSize()
{
	CalcTextTotalSize( m_flTextWorldWidth, m_flTextWorldHeight );
}

void PointWorldText::CalcTextTotalSize(float &outWidth, float &outHeight)
{
	outWidth = 0.0f;
	outHeight = 0.0f;

	const char *szText = m_szText;
	if ( !szText[0] )
		return;

	int nNumChars = m_nTextLength;
	if ( !nNumChars )
		return;

	float screenSize = m_flTextSize;
	float screenSpacingX = GetTextSpacingX();
	float screenSpacingY = GetTextSpacingY();
	Font* font = &font_Roboto_Mono;
	outHeight += font->size;
	float flLineWidth = 0.0f;
	for ( int i = 0; i < nNumChars; i++ )
	{
		char nChar = *(szText++);
		unsigned int nCharIdx = Clamp<unsigned int>( ( unsigned int )( nChar ) - 32, 0u, ( unsigned int )( ARRAYSIZE( characters_Roboto_Mono ) - 1u ) );
		Character *character = &font->characters[ nCharIdx ];
		float scale = screenSize / (float)font->size;
		if ( nChar == '\n' )
		{
			outWidth = Max( outWidth, flLineWidth );
			flLineWidth = 0.0f;
			outHeight += (font->size + screenSpacingY) * scale;
			continue;
		}
		flLineWidth += (character->advance + screenSpacingX) * scale;
	}
	outWidth = Max( outWidth, flLineWidth );
}

float PointWorldText::GetTextWorldWidth() const
{
	return m_flTextWorldWidth;
}
float PointWorldText::GetTextWorldHeight() const
{
	return m_flTextWorldHeight;
}
float PointWorldText::GetTextSpacingX() const
{
	return m_flTextSpacingX;
}
float PointWorldText::GetTextSpacingY() const
{
	return m_flTextSpacingY;
}

void PointWorldText::DrawModel()
{
	const char *szText = m_szText;
	if (!szText[0])
		return;

	int nNumChars = m_nTextLength;
	if (!nNumChars)
		return;

	IMaterial* pDebugText = *m_Font;
	if (!pDebugText)
		return;

	if (m_colTextColor.a <= 0)
		return;

	Vector ViewForward( 1.0f, 0.0f, 0.0f );
	Vector ViewUp( 0.0f, 1.0f, 0.0f );
	Vector ViewRight( 0.0f, 0.0f, -1.0f );
	Vector vecStartPos;
	VectorCopy( GetAbsOrigin(), vecStartPos );

	float screenSize = m_flTextSize;
	float screenSpacingX = GetTextSpacingX();
	float screenSpacingY = GetTextSpacingY();

	switch ( m_nOrientation )
	{
		case POINTWORLDTEXTORIENTATION_VIEW_DIRECTION:
			ViewForward = -CurrentViewForward();
			ViewUp = CurrentViewUp();
			ViewRight = CurrentViewRight();
			vecStartPos -= GetTextWorldWidth() * 0.5f * ViewRight;
			break;
		case POINTWORLDTEXTORIENTATION_VIEW_DIRECTION_Z_ALIGNED:
			ViewForward = -CurrentViewForward();
			ViewUp = Vector(0, 0, 1);
			ViewRight = CurrentViewRight();
			// center the text for nicer rotation
			vecStartPos -= GetTextWorldWidth() * 0.5f * ViewRight;
			break;
		case POINTWORLDTEXTORIENTATION_VIEW_ORIGIN_Z_ALIGNED:
			ViewForward = -CurrentViewOrigin() + GetAbsOrigin();
			ViewUp = Vector(0, 0, 1);
			ViewRight = ViewForward.Cross(ViewUp).Normalized();
			// center the text for nicer rotation
			vecStartPos -= GetTextWorldWidth() * 0.5f * ViewRight;
			break;
		case POINTWORLDTEXTORIENTATION_ENTITY_ORIENTATION:
		default:
			AngleVectors( GetAbsAngles(), &ViewForward, &ViewRight, &ViewUp );
			break;
	}

	Vector vecOrigStartPos = vecStartPos;
	{
		Vector screen;
		if (bool behind = ScreenTransform(vecStartPos, screen);
			behind)
		{
			Vector vecEndPos = vecStartPos + (ViewRight * GetTextWorldWidth());
			if (bool behind = ScreenTransform(vecEndPos, screen);
				behind)
			{
				return;
			}
		}
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->Bind( pDebugText );
	pDebugText->IncrementReferenceCount();

	IMesh* pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nNumChars );

	Font *font = &font_Roboto_Mono;
	color32 color = m_colTextColor;
	byte* pColor = (byte*)&color;

	for ( int i = 0; i < nNumChars; i++ )
	{
		char nChar = *(szText++);
		unsigned int nCharIdx = Clamp( ( unsigned int )( nChar ) - 32, 0u, ( unsigned int )( ARRAYSIZE( characters_Roboto_Mono ) - 1u ) );
		Character *character = &font->characters[ nCharIdx ];
		float scale = screenSize / (float)font->size;
		if ( nChar == '\n' )
		{
			vecOrigStartPos -= ( ViewUp * ( (font->size + screenSpacingY) * scale ) );
			vecStartPos = vecOrigStartPos;
			continue;
		}
		if ( nChar != ' ' )
		{
			float x, y, s, t;

			x = -character->originX;
			y = -character->originY;
			s = character->x / (float)font->width;
			t = character->y / (float)font->height;
			Vector v0 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v0.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();

			x = -character->originX;
			y = -character->originY + character->height;
			s = character->x / (float)font->width;
			t = (character->y + character->height) / (float)font->height;
			Vector v2 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v2.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();

			x = -character->originX + character->width;
			y = -character->originY + character->height;
			s = (character->x + character->width) / (float)font->width;
			t = (character->y + character->height) / (float)font->height;
			Vector v3 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v3.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();

			x = -character->originX + character->width;
			y = -character->originY;
			s = (character->x + character->width) / (float)font->width;
			t = (character->y) / (float)font->height;
			Vector v1 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v1.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();
		}
		vecStartPos += ViewRight * ((character->advance + screenSpacingX) * scale);
	}
	meshBuilder.End();
	pMesh->Draw();
	return;
}