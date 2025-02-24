//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "hudelement.h"
#include "clienteffectprecachesystem.h"
#include "sourcevr/isourcevirtualreality.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: HDU Damage indication
//-----------------------------------------------------------------------------
class CNEOHudDamageIndicator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHudDamageIndicator, vgui::Panel);

public:
	CNEOHudDamageIndicator(const char* pElementName);
	void Init(void);
	void Reset(void);
	virtual bool ShouldDraw(void);

	// Handler for our message
	void MsgFunc_Damage(bf_read& msg);

private:
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

private:
	CPanelAnimationVarAliasType(float, m_flDmgX, "dmg_xpos", "10", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flDmgY, "dmg_ypos", "80", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flDmgWide, "dmg_wide", "30", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flDmgTall1, "dmg_tall1", "300", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flDmgTall2, "dmg_tall2", "240", "proportional_float");

	CPanelAnimationVar(Color, m_DmgFullscreenColor, "DmgFullscreenColor", "255 88 0 25");

	void DrawFullscreenDamageIndicator();

	CMaterialReference m_WhiteAdditiveMaterial;
};

DECLARE_HUDELEMENT(CNEOHudDamageIndicator);
DECLARE_HUD_MESSAGE(CNEOHudDamageIndicator, Damage);

struct DamageAnimation_t
{
	const char* name;
	int bitsDamage;
	int damage;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CNEOHudDamageIndicator::CNEOHudDamageIndicator(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudDamageIndicator")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_WhiteAdditiveMaterial.Init("vgui/white_additive", TEXTURE_GROUP_VGUI);

	SetHiddenBits(HIDEHUD_HEALTH);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHudDamageIndicator::Reset(void)
{
	m_DmgFullscreenColor[3] = 0;
}

void CNEOHudDamageIndicator::Init(void)
{
	HOOK_HUD_MESSAGE(CNEOHudDamageIndicator, Damage);
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CNEOHudDamageIndicator::ShouldDraw(void)
{
	bool bNeedsDraw = m_DmgFullscreenColor[3];

	return (bNeedsDraw && CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
// Purpose: Draws full screen damage fade
//-----------------------------------------------------------------------------
void CNEOHudDamageIndicator::DrawFullscreenDamageIndicator()
{
	CMatRenderContextPtr pRenderContext(materials);
	IMesh* pMesh = pRenderContext->GetDynamicMesh(true, NULL, NULL, m_WhiteAdditiveMaterial);

	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);
	int r = m_DmgFullscreenColor[0], g = m_DmgFullscreenColor[1], b = m_DmgFullscreenColor[2], a = m_DmgFullscreenColor[3];

	float wide = GetWide(), tall = GetTall();

	meshBuilder.Color4ub(r, g, b, a);
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.Position3f(0.0f, 0.0f, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(r, g, b, a);
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.Position3f(wide, 0.0f, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(r, g, b, a);
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.Position3f(wide, tall, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub(r, g, b, a);
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.Position3f(0.0f, tall, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Paints the damage display
//-----------------------------------------------------------------------------
void CNEOHudDamageIndicator::Paint()
{
	// draw fullscreen damage indicators
	DrawFullscreenDamageIndicator();
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void CNEOHudDamageIndicator::MsgFunc_Damage(bf_read& msg)
{
	int armor = msg.ReadByte();	// armor
	int damageTaken = msg.ReadByte();	// health
	long bitsDamage = msg.ReadLong(); // damage bits

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (pPlayer->GetHealth() <= 0)
		return;

	if (damageTaken > 0 || armor > 0)
	{
		// see which effect to play
		if (bitsDamage & DMG_BULLET || bitsDamage & DMG_SLASH || bitsDamage & DMG_CLUB)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HudTakeDamageFront");
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CNEOHudDamageIndicator::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport(vx, vy, vw, vh);

	SetForceStereoRenderToFrameBuffer(true);

	if (UseVR())
	{
		m_flDmgY = 0.125f * (float)vh;
		m_flDmgTall1 = 0.625f * (float)vh;
		m_flDmgTall2 = 0.4f * (float)vh;
		m_flDmgWide = 0.1f * (float)vw;
	}

	SetSize(vw, vh);
}
