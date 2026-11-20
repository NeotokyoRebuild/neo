#ifndef NEO_HUD_AMMO_H
#define NEO_HUD_AMMO_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

class CNEOHud_Ammo : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Ammo, EditablePanel);

public:
	CNEOHud_Ammo(const char *pElementName, vgui::Panel *parent = nullptr);

	virtual ~CNEOHud_Ammo() { }

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawAmmo() const;
	void DrawWeaponIcon(const C_NEOBaseCombatWeapon *activeWep, int yBaseline) const;
	int DrawReserve(wchar_t wchGlyph, const wchar_t *pwszCount, int iRight, int yBaseline) const;
	int DrawSupaReserves(int iShells, int iSlugs, int iRight, int yBaseline) const;
	void DrawWeaponName(const wchar_t *pwszName, int yBaseline, int iRoomRight) const;
	void DrawRounds(wchar_t wchBullet, int iMagSizeMax, int iMagSizeCurrent, int yBaseline) const;
	void DrawHeatMeter(C_NEOBaseCombatWeapon* activeWep, int yBaseline) const;
	vgui::HFont GetBulletFont(wchar_t wchBullet) const;
	vgui::HFont GetReserveGlyphFont(wchar_t wchGlyph) const;

private:
	vgui::HFont m_hSmallTextFont = 0;
	vgui::HFont m_hCountFont = 0;
	vgui::HFont m_hLabelFont = 0;
	vgui::HFont m_hLabelSmallFont = 0;
	vgui::HFont m_hStatusFont = 0;
	vgui::HFont m_hBulletFont = 0;
	vgui::HFont m_hFireModeFont = 0;
	vgui::HFont m_hThrowableIconFont = 0;
	vgui::HFont m_hWeaponIconFont = 0;
	vgui::HFont m_hWeaponIconSmallFont = 0;

	// Every round glyph is drawn the same height, so each needs its own font size
	struct BulletFont
	{
		wchar_t wchBullet;
		vgui::HFont hFont;
	};
	BulletFont m_bulletFonts[6] = {};

	// The standing round beside a mag count: the rail's sizes, scaled down to the numerals
	BulletFont m_reserveGlyphFonts[5] = {};

	int m_resX, m_resY;

	CPanelAnimationVarAliasType(int, xpos, "xpos", "r203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "443", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, tall, "tall", "35", "proportional_ypos");
	CPanelAnimationVarAliasType(int, visible, "visible", "1", "int");
	CPanelAnimationVarAliasType(int, enabled, "enabled", "1", "int");
	CPanelAnimationVar(Color, box_color, "box_color", "200 200 200 40");

	CPanelAnimationVarAliasType(bool, top_left_corner, "top_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, top_right_corner, "top_right_corner", "0", "bool");
	CPanelAnimationVarAliasType(bool, bottom_left_corner, "bottom_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, bottom_right_corner, "bottom_right_corner", "0", "bool");

	CPanelAnimationVarAliasType(int, text_xpos, "text_xpos", "34", "proportional_xpos");
	CPanelAnimationVarAliasType(int, text_ypos, "text_ypos", "4", "proportional_ypos");
	CPanelAnimationVarAliasType(int, digit_xpos, "digit_xpos", "34", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit_ypos, "digit_ypos", "24", "proportional_ypos");
	CPanelAnimationVarAliasType(int, row_h, "row_h", "6", "proportional_ypos");
	CPanelAnimationVarAliasType(bool, digit_as_number, "digit_as_number", "0", "bool");
	CPanelAnimationVarAliasType(int, count_gap, "count_gap", "14", "proportional_xpos");
	CPanelAnimationVarAliasType(int, count_xpos, "count_xpos", "197", "proportional_xpos");
	CPanelAnimationVarAliasType(int, icon_xpos, "icon_xpos", "6", "proportional_xpos");
	CPanelAnimationVarAliasType(int, icon_wide, "icon_wide", "22", "proportional_xpos");
	CPanelAnimationVar(Color, ammo_text_color, "ammo_text_color", "255 255 255 100");
	CPanelAnimationVar(Color, ammo_color, "ammo_color", "255 255 255 150");
	CPanelAnimationVar(Color, emptied_ammo_color, "emptied_ammo_color", "255 255 255 50");
	CPanelAnimationVar(Color, count_color, "count_color", "255 255 255 150");

	CPanelAnimationVarAliasType(int, heatbar_xpos, "heatbar_xpos", "34", "proportional_xpos");
	CPanelAnimationVarAliasType(int, heatbar_ypos, "heatbar_ypos", "14", "proportional_ypos");
	CPanelAnimationVarAliasType(float, heatbar_w, "heatbar_w", "163", "proportional_float");
	CPanelAnimationVarAliasType(float, heatbar_h, "heatbar_h", "16", "proportional_float");
	CPanelAnimationVar(Color, heat_color, "heat_color", "255 50 50 200");

private:
	CNEOHud_Ammo(const CNEOHud_Ammo& other);
};

#endif // NEO_HUD_AMMO_H
