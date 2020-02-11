#ifndef NEO_HUD_CHILDELEMENT_H
#define NEO_HUD_CHILDELEMENT_H
#ifdef _WIN32
#pragma once
#endif

class CNeoHudElements;
class C_NEO_Player;

class CNEOHud_ChildElement
{
	DECLARE_CLASS_NOBASE(CNEOHud_ChildElement)

public:
	CNEOHud_ChildElement();
	virtual ~CNEOHud_ChildElement() { }

protected:
	void SetLastHudUpdater(C_NEO_Player* player);
	bool IsHudReadyForPaintNow() const;

private:
	CNeoHudElements* m_pNeoHud;

private:
	CNEOHud_ChildElement(CNEOHud_ChildElement& other);
};

#endif // NEO_HUD_CHILDELEMENT_H