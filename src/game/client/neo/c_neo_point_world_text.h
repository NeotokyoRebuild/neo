#pragma once

///////////////////////////////////////////////
// A non-networked non-entity PointWorldText //
///////////////////////////////////////////////

#include "neo_player_shared.h"

enum PointWorldTextOrientation
{
	POINTWORLDTEXTORIENTATION_ENTITY_ORIENTATION = 0,
	POINTWORLDTEXTORIENTATION_VIEW_DIRECTION,
	POINTWORLDTEXTORIENTATION_VIEW_DIRECTION_Z_ALIGNED,
	POINTWORLDTEXTORIENTATION_VIEW_ORIGIN_Z_ALIGNED,

	POINTWORLDTEXTORIENTATION__TOTAL
};

class PointWorldText
{
public:
	PointWorldText();
	PointWorldText(char* pszText, Vector pos, CMaterialReference* font);
	~PointWorldText();

	void DrawModel();

	void SetAbsOrigin(Vector origin) { m_vecAbsOrigin = origin; };
	void SetText(const char* pszText);
	void SetAlpha(const float alpha) { m_colTextColor.a = alpha; };
	void SetTextSize(const float size) { m_flTextSize = size; UpdateTextWorldSize(); };
	void SetTextSpacingX(const float spacing) { m_flTextSpacingX = spacing; UpdateTextWorldSize(); };
	void SetOrientation(const PointWorldTextOrientation orientation) { m_nOrientation = orientation; };

	Vector GetAbsOrigin() { return m_vecAbsOrigin; }
	QAngle GetAbsAngles() { return m_vecAbsAngles; }

private:
	void CalcTextTotalSize(float &outWidth, float &outHeight);
	void UpdateTextWorldSize();

	float GetTextWorldWidth() const;
	float GetTextWorldHeight() const;
	float GetTextSpacingX() const;
	float GetTextSpacingY() const;

	Vector m_vecAbsOrigin = {0, 0, 0};
	QAngle m_vecAbsAngles = {0, 0, 0};

	float m_flTextSize = 64.f;
	float m_flTextSpacingX = 0.f;
	float m_flTextSpacingY = 0.f;
	color32 m_colTextColor = {(byte)COLOR_NEO_WHITE.r(), (byte)COLOR_NEO_WHITE.g(), (byte)COLOR_NEO_WHITE.b(), (byte)COLOR_NEO_WHITE.a()};
	PointWorldTextOrientation m_nOrientation = POINTWORLDTEXTORIENTATION_VIEW_ORIGIN_Z_ALIGNED;
	
	char m_szText[ MAX_PLACE_NAME_LENGTH ];
	int m_nTextLength = 0;
	float m_flTextWorldWidth = 0.f;
	float m_flTextWorldHeight = 0.f;
	
	CMaterialReference* m_Font;
};