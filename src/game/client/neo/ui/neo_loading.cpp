#include "neo_loading.h"

#include "cbase.h"
#include "ienginevgui.h"
#include "ui/neo_root.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
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

	vgui::ivgui()->AddTickSignal(GetVPanel(), 250);
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

	const bool bIsActivate = (V_strcmp(pSzMsgName, "Activate") == 0);
	const bool bIsDeactivate = (V_strcmp(pSzMsgName, "deactivate") == 0);

	if (bIsActivate || bIsDeactivate)
	{
		g_pNeoRoot->m_bOnLoadingScreen = bIsActivate;
		V_memset(&m_info, 0, sizeof(CNeoLoading::LoadingInfos));
	}
}

CNeoLoading::RetGameUIPanels CNeoLoading::FetchGameUIPanels()
{
	RetGameUIPanels panels = {}; // Zero-init

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
		return panels;
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
		return panels;
	}

	panels.pLoadingPanel = static_cast<vgui::Frame *>(vgui::ipanel()->GetPanel(loadingPanel, "GAMEUI"));
	panels.aStrIdxMap[LOADINGSTATE_LOADING] = g_pVGuiLocalize->FindIndex("GameUI_Loading");
	panels.aStrIdxMap[LOADINGSTATE_DISCONNECTED] = g_pVGuiLocalize->FindIndex("GameUI_Disconnected");

	// Hide loading panel
	// NEO NOTE (nullsystem): Do not just un-parent it as that'll make
	// it unable to pick up on next OnThink. Instead just make it look invisible.
	panels.pLoadingPanel->SetAlpha(0);
	panels.pLoadingPanel->SetSize(0, 0);
	panels.pLoadingPanel->SetBgColor(COLOR_TRANSPARENT);
	panels.pLoadingPanel->SetFgColor(COLOR_TRANSPARENT);
	panels.pLoadingPanel->SetMouseInputEnabled(false);
	panels.pLoadingPanel->SetKeyBoardInputEnabled(false);
	vgui::TextImage *pTITitle = panels.pLoadingPanel->TITitlePtr();
	if (pTITitle)
	{
		pTITitle->SetColor(COLOR_TRANSPARENT);
	}

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
		pPanel->SetAlpha(0);
		if (V_strcmp(curLoadChPanelName, "Progress") == 0)
		{
			Assert(V_strcmp(curLoadChPanelClass, "ProgressBar") == 0);
			panels.pProgressBarMain = static_cast<vgui::ProgressBar *>(pPanel);
		}
		else if (V_strcmp(curLoadChPanelName, "InfoLabel") == 0)
		{
			Assert(V_strcmp(curLoadChPanelClass, "Label") == 0);
			panels.pLabelInfo = static_cast<vgui::Label *>(pPanel);
		}
		// NEO NOTE (nullsystem): Unused panels:
		//    "Progress2" - Don't seem utilized
		//    "TimeRemainingLabel" - Don't seem utilized
		//    "CancelButton" - Can't do mouse input proper/workaround doesn't work well
	}

	return panels;
}

void CNeoLoading::Paint()
{
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

	// Only deals with paint as key inputs already dealt with by SDK
	NeoUI::BeginContext(&m_uiCtx, NeoUI::MODE_PAINT, m_info.wszTitle, "NeoLoadingMainCtx");
	if (m_info.iStrIdx == m_info.aStrIdxMap[LOADINGSTATE_LOADING])
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
			NeoUI::Label(m_info.wszLabelInfo);
			NeoUI::Progress(m_info.flProgressBarMain, 0.0f, 1.0f);
		}
		NeoUI::EndSection();
	}
	else if (m_info.iStrIdx == m_info.aStrIdxMap[LOADINGSTATE_DISCONNECTED])
	{
		m_uiCtx.dPanel.tall = (m_iRowsInScreen / 2) * m_uiCtx.layout.iRowTall;
		m_uiCtx.dPanel.y = (tall / 2) - (m_uiCtx.dPanel.tall / 2);
		m_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
		NeoUI::BeginSection(true);
		{
			NeoUI::LabelWrap(m_info.wszLabelInfo);
			NeoUI::Pad();
			NeoUI::Label(L"Press ESC to go back");
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoLoading::OnThink()
{
	BaseClass::OnThink();

	if (!g_pNeoRoot->m_bOnLoadingScreen)
	{
		return;
	}

	const RetGameUIPanels panels = FetchGameUIPanels();
	if (!panels.pLoadingPanel)
	{
		return;
	}

	DevMsg("CNeoLoading::OnThink: VALID PANEL");

	for (int i = 0; i < LOADINGSTATE__TOTAL; ++i)
	{
		m_info.aStrIdxMap[i] = panels.aStrIdxMap[i];
	}

	// NEO JANK (nullsystem): Since we don't have proper access to loading internals,
	// determining by localization text index should be good enough to differ between
	// loading and disconnect state.
	vgui::TextImage *pTITitle = panels.pLoadingPanel->TITitlePtr();

	m_info.iStrIdx = pTITitle ? pTITitle->GetUnlocalizedTextSymbol() : INVALID_LOCALIZE_STRING_INDEX;
	V_wcscpy_safe(m_info.wszTitle, pTITitle ? pTITitle->GetUText() : L"Loading...");
	m_info.flProgressBarMain = (panels.pProgressBarMain) ? panels.pProgressBarMain->GetProgress() : 0.0f;
	V_wcscpy_safe(m_info.wszLabelInfo, (panels.pLabelInfo) ? panels.pLabelInfo->GetTextImage()->GetUText() : L"");
}

