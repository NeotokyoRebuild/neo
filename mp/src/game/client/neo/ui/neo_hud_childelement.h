#ifndef NEO_HUD_CHILDELEMENT_H
#define NEO_HUD_CHILDELEMENT_H
#ifdef _WIN32
#pragma once
#endif

class CNeoHudElements;
class C_NEO_Player;

#define NEO_HUD_ELEMENT_FREQ_CVAR_NAME(Name) cl_neo_hud_ ## Name ## _update_freq
#ifndef xstr
#define xstr(a) str(a)
#endif
#ifndef str
#define str(a) #a
#endif
#define NEO_HUD_ELEMENT_FREQ_CVAR_DESCRIPTION "How often to update this HUD element, in seconds. 0: always update, <0: never update."
#define NEO_HUD_ELEMENT_FREQ_CVAR_MINMAX_PARMS true, -1.0, true, 1.0
#define NEO_HUD_ELEMENT_FREQ_CVAR_FLAGS (FCVAR_ARCHIVE | FCVAR_USERINFO)
#define NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(HudElementId, DefaultUpdateFrequencyInSeconds) ConVar NEO_HUD_ELEMENT_FREQ_CVAR_NAME(HudElementId)(xstr(NEO_HUD_ELEMENT_FREQ_CVAR_NAME(HudElementId)), #DefaultUpdateFrequencyInSeconds, NEO_HUD_ELEMENT_FREQ_CVAR_FLAGS, NEO_HUD_ELEMENT_FREQ_CVAR_DESCRIPTION, NEO_HUD_ELEMENT_FREQ_CVAR_MINMAX_PARMS); \
	ConVar* CNEOHud_ ## HudElementId::GetUpdateFrequencyConVar() const { return &NEO_HUD_ELEMENT_FREQ_CVAR_NAME(HudElementId); }

extern ConVar neo_cl_hud_ammo_enabled;

class CNEOHud_ChildElement
{
	DECLARE_CLASS_NOBASE(CNEOHud_ChildElement)

public:
	CNEOHud_ChildElement();
	virtual ~CNEOHud_ChildElement() { }

protected:
	virtual void DrawNeoHudRoundedBox(const int x0, const int y0, const int x1, const int y1) const;

	virtual void UpdateStateForNeoHudElementDraw() = 0;
	virtual void DrawNeoHudElement() = 0;
	virtual ConVar* GetUpdateFrequencyConVar() const = 0;

	void PaintNeoElement()
	{
		if (!engine->IsDrawingLoadingImage())
		{
			if (ShouldUpdateYet())
			{
				UpdateStateForNeoHudElementDraw();
			}

			DrawNeoHudElement();
		}
	}

	bool ShouldUpdateYet()
	{
		const float frequency = GetUpdateFrequency();
		if (frequency < 0)
		{
			return false;
		}
		else if (frequency > 0)
		{
			const float deltaTime = gpGlobals->curtime - m_flLastUpdateTime;
			if (deltaTime < frequency)
			{
				return false;
			}
			else
			{
				m_flLastUpdateTime = gpGlobals->curtime;
				return true;
			}
		}
		else
		{
			return true;
		}
	}

	CNeoHudElements* GetRootNeoHud() const { return m_pNeoHud; }

private:
	float GetUpdateFrequency() const { return GetUpdateFrequencyConVar()->GetFloat(); }

private:
	CNeoHudElements* m_pNeoHud;

	vgui::HTexture m_hTex_Rounded_NE, m_hTex_Rounded_NW, m_hTex_Rounded_SE, m_hTex_Rounded_SW;
	int m_rounded_width, m_rounded_height;

	float m_flLastUpdateTime;

private:
	CNEOHud_ChildElement(CNEOHud_ChildElement& other);
};

#endif // NEO_HUD_CHILDELEMENT_H
