#include "cbase.h"
#include "IOverrideInterface.h"
#include "neo_root.h"

// Derived class of override interface
class COverrideInterface : public IOverrideInterface
{
private:
	CNeoRoot *MainMenu = nullptr;
 
public:
	void Create(vgui::VPANEL parent) override
	{
		// Create immediately
		MainMenu = new CNeoRoot(parent);
	}

	vgui::VPANEL GetPanel() override
	{
		if (!MainMenu) return 0U;
		return MainMenu->GetVPanel();
	}
 
	void Destroy() override
	{
		if (MainMenu)
		{
			MainMenu->SetParent(nullptr);
			delete MainMenu;
		}
	}
};
 
static COverrideInterface g_SMenu;
IOverrideInterface *OverrideUI = (IOverrideInterface *)&g_SMenu;
