#include "cbase.h"
#include "IOverrideInterface.h"
#include "neo_root.h"

// Derived class of override interface
class COverrideInterface : public IOverrideInterface
{
private:
	CNeoRoot *MainMenu;
 
public:
	void Create( vgui::VPANEL parent )
	{
		// Create immediately
		MainMenu = new CNeoRoot(parent);
	}

	vgui::VPANEL GetPanel( void )
	{
		if ( !MainMenu )
			return 0U;
		return MainMenu->GetVPanel();
	}
 
	void Destroy( void )
	{
		if ( MainMenu )
		{
			MainMenu->SetParent( (vgui::Panel *)NULL );
			delete MainMenu;
		}
	}
 
};
 
static COverrideInterface g_SMenu;
IOverrideInterface *OverrideUI = ( IOverrideInterface * )&g_SMenu;
