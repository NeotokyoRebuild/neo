#include "neo_loading.h"

#include "cbase.h"
#include "ienginevgui.h"
#include "ui/neo_root.h"
#include "vgui/ISurface.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Frame.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CNeoLoading::CNeoLoading()
	: vgui::EditablePanel(nullptr, "NeoLoadingPanel")
{
	SetParent(enginevgui->GetPanel(PANEL_GAMEUIDLL));
	InvalidateLayout(false, true);
	SetVisible(false);
	MakePopup(false);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(neoscheme);

	SetMouseInputEnabled(true);
	// NEO TODO (nullsystem): Don't need to handle KB input since ESC already does that
	// Unless console controller the concern?
	//SetKeyBoardInputEnabled(true);

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(neoscheme);
	ApplySchemeSettings(pScheme);

	m_pHostMap = g_pCVar->FindVar("host_map");
	Assert(m_pHostMap != nullptr);
}

CNeoLoading::~CNeoLoading()
{
	NeoUI::FreeContext(&m_uiCtx);
}

void CNeoLoading::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char *FONT_NAMES[NeoUI::FONT__TOTAL] = {
		"NeoUINormal", "NHudOCR", "NHudOCRSmallNoAdditive", "ClientTitleFont", "ClientTitleFontSmall",
		"NeoUILarge"
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		m_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	ResetSizes(wide, tall);
}

void CNeoLoading::ResetSizes(const int wide, const int tall)
{
	// In 1080p, g_uiCtx.layout.iDefRowTall == 40, g_uiCtx.iMarginX = 10, g_iAvatar = 64,
	// other resolution scales up/down from it
	m_uiCtx.layout.iDefRowTall = tall / 27;
	m_iRowsInScreen = (tall * 0.85f) / m_uiCtx.layout.iDefRowTall;
	m_uiCtx.iMarginX = wide / 192;
	m_uiCtx.iMarginY = tall / 108;
	m_uiCtx.selectBgColor = COLOR_NEOPANELACCENTBG;
	const float flWide = static_cast<float>(wide);
	float flWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (flWideAs43 > flWide) flWideAs43 = flWide;
	m_iRootSubPanelWide = static_cast<int>(flWideAs43 * 0.9f);
}

void CNeoLoading::OnMessage(const KeyValues *params, vgui::VPANEL fromPanel)
{
	BaseClass::OnMessage(params, fromPanel);
	const char *pSzMsgName = params->GetName();
	g_pNeoRoot->m_flTimeLoadingScreenTransition = gpGlobals->realtime;
	if (V_strcmp(pSzMsgName, "Activate") == 0)
	{
		FetchGameUIPanels();
		g_pNeoRoot->m_bOnLoadingScreen = true;
	}
	else if (V_strcmp(pSzMsgName, "deactivate") == 0)
	{
		g_pNeoRoot->m_bOnLoadingScreen = false;
		if (engine->IsConnected() && !engine->IsLevelMainMenuBackground())
		{
			g_pNeoRoot->m_flTimeLoadingScreenTransition -= (NEO_MENU_SECONDS_DELAY + NEO_MENU_SECONDS_TILL_FULLY_OPAQUE); // Don't fade in the menu on disconnect
		}
	}
}

void CNeoLoading::FetchGameUIPanels()
{
	m_bValidGameUIPanels = false;
	vgui::VPANEL basePanel = vgui::INVALID_PANEL;
	vgui::VPANEL rootPanel = enginevgui->GetPanel(PANEL_GAMEUIDLL);
	const int iRootChildren = vgui::ipanel()->GetChildCount(rootPanel);
	for (int i = 0; i < iRootChildren; ++i)
	{
		const vgui::VPANEL curBasePanel = vgui::ipanel()->GetChild(rootPanel, i);
		const char *curBasePanelName = vgui::ipanel()->GetName(curBasePanel);
		if (V_strcmp(curBasePanelName, "BaseGameUIPanel") == 0)
		{
			basePanel = curBasePanel;
			break;
		}
	}
	if (basePanel == vgui::INVALID_PANEL)
	{
		return;
	}

	vgui::VPANEL loadingPanel = vgui::INVALID_PANEL;
	const int iBaseChildren = vgui::ipanel()->GetChildCount(basePanel);
	for (int i = 0; i < iBaseChildren; ++i)
	{
		const vgui::VPANEL curLoadingPanel = vgui::ipanel()->GetChild(basePanel, i);
		const char *curLoadingPanelName = vgui::ipanel()->GetName(curLoadingPanel);
		if (V_strcmp(curLoadingPanelName, "LoadingDialog") == 0)
		{
			loadingPanel = curLoadingPanel;
			break;
		}
	}
	if (loadingPanel == vgui::INVALID_PANEL)
	{
		return;
	}

	m_pLoadingPanel = static_cast<vgui::Frame *>(vgui::ipanel()->GetPanel(loadingPanel, "GAMEUI"));

	m_aStrIdxMap[LOADINGSTATE_LOADING] = g_pVGuiLocalize->FindIndex("GameUI_Loading");
	m_aStrIdxMap[LOADINGSTATE_DISCONNECTED] = g_pVGuiLocalize->FindIndex("GameUI_Disconnected");

	// Hide loading panel
	vgui::ipanel()->SetParent(loadingPanel, 0);
	m_bValidGameUIPanels = true;
	const int iLoadingChildren = vgui::ipanel()->GetChildCount(loadingPanel);
	for (int i = 0; i < iLoadingChildren; ++i)
	{
		const vgui::VPANEL curLoadChPanel = vgui::ipanel()->GetChild(loadingPanel, i);
		const char *curLoadChPanelName = vgui::ipanel()->GetName(curLoadChPanel);
		const char *curLoadChPanelClass = vgui::ipanel()->GetClassName(curLoadChPanel);
		Panel *pPanel = vgui::ipanel()->GetPanel(curLoadChPanel, "GAMEUI");
		if (!pPanel)
		{
			continue;
		}
		if (V_strcmp(curLoadChPanelName, "Progress") == 0)
		{
			Assert(V_strcmp(curLoadChPanelClass, "ProgressBar") == 0);
			m_pProgressBarMain = static_cast<vgui::ProgressBar *>(pPanel);
		}
		else if (V_strcmp(curLoadChPanelName, "InfoLabel") == 0)
		{
			Assert(V_strcmp(curLoadChPanelClass, "Label") == 0);
			m_pLabelInfo = static_cast<vgui::Label *>(pPanel);
		}
		// NEO NOTE (nullsystem): Unused panels:
		//    "Progress2" - Don't seem utilized
		//    "TimeRemainingLabel" - Don't seem utilized
		//    "CancelButton" - Can't do mouse input proper/workaround doesn't work well
	}
}

void CNeoLoading::Paint()
{
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNeoLoading::OnMainLoop(const NeoUI::Mode eMode)
{
	// NEO JANK (nullsystem): Since we don't have proper access to loading internals,
	// determining by localization text index should be good enough to differ between
	// loading and disconnect state.
	vgui::TextImage* pTITitle = m_pLoadingPanel ? m_pLoadingPanel->TITitlePtr() : nullptr;
	const StringIndex_t iStrIdx = pTITitle ? pTITitle->GetUnlocalizedTextSymbol() : INVALID_LOCALIZE_STRING_INDEX;

	static bool bStaticInitNeoUI = false;
	bool bSkipRender = false;
	if (iStrIdx == m_aStrIdxMap[LOADINGSTATE_LOADING] && m_pHostMap)
	{
		auto hostMapName = m_pHostMap->GetString();
		if (Q_stristr(hostMapName, "background_"))
		{
			bSkipRender = true;
		}

		// Check whether current background corresponds to the map we're loading into here?

		if (bSkipRender && bStaticInitNeoUI)
		{
			return;
		}
	}

	static constexpr int BOTTOM_ROWS = 3;

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	int panWide, panTall;
	GetSize(panWide, panTall);
	if (panWide != wide || panTall != tall)
	{
		SetSize(wide, tall);
		ResetSizes(wide, tall);
	}
	m_uiCtx.dPanel.x = (wide / 2) - (m_iRootSubPanelWide / 2);
	m_uiCtx.dPanel.wide = m_iRootSubPanelWide;
	m_uiCtx.dPanel.tall = (m_iRowsInScreen - BOTTOM_ROWS) * m_uiCtx.layout.iRowTall;
	m_uiCtx.dPanel.y = (tall / 2) - ((m_iRowsInScreen * m_uiCtx.layout.iRowTall) / 2);
	m_uiCtx.bgColor = COLOR_TRANSPARENT;

	NeoUI::BeginContext(&m_uiCtx, eMode, pTITitle ? pTITitle->GetUText() : L"Loading...", "NeoLoadingMainCtx");
	if (bSkipRender)
	{
		NeoUI::BeginSection();
		NeoUI::EndSection();
		bStaticInitNeoUI = true;
	}
	else
	{
		if (iStrIdx == m_aStrIdxMap[LOADINGSTATE_LOADING])
		{
			NeoUI::BeginSection();
			{
				m_uiCtx.eFont = NeoUI::FONT_NTLARGE;
				NeoUI::Label(m_wszLoadingMap);
				m_uiCtx.eFont = NeoUI::FONT_NTNORMAL;
			}
			NeoUI::EndSection();
			m_uiCtx.dPanel.y += m_uiCtx.dPanel.tall;
			m_uiCtx.dPanel.tall = BOTTOM_ROWS * m_uiCtx.layout.iRowTall;
			m_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
			NeoUI::BeginSection(true);
			{
				NeoUI::Label(L"Press ESC to cancel");
				if (m_pLabelInfo) NeoUI::Label(m_pLabelInfo->GetTextImage()->GetUText());
				if (m_pProgressBarMain) NeoUI::Progress(m_pProgressBarMain->GetProgress(), 0.0f, 1.0f);
			}
			NeoUI::EndSection();
		}
		else if (iStrIdx == m_aStrIdxMap[LOADINGSTATE_DISCONNECTED])
		{
			m_uiCtx.dPanel.tall = (m_iRowsInScreen / 2) * m_uiCtx.layout.iRowTall;
			m_uiCtx.dPanel.y = (tall / 2) - (m_uiCtx.dPanel.tall / 2);
			m_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
			NeoUI::BeginSection(true);
			{
				if (m_pLabelInfo) NeoUI::LabelWrap(m_pLabelInfo->GetTextImage()->GetUText());
				NeoUI::Pad();
				NeoUI::Label(L"Press ESC to go back");
			}
			NeoUI::EndSection();
		}
	}
	NeoUI::EndContext();
}
