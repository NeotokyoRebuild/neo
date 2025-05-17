#pragma once

#include "ui/neo_ui.h"
#include <vgui_controls/EditablePanel.h>

/*
 * Override loading screen
 *
 * Based on: https://developer.valvesoftware.com/wiki/Custom_loading_screen by Maestra Fenix
 * Although goes beyond and tries to re-route text and progress into NeoUI
 */

class CNeoLoading : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNeoLoading, vgui::EditablePanel);
public:
	CNeoLoading();
	~CNeoLoading();
	wchar_t m_wszLoadingMap[256] = {};

	void ApplySchemeSettings(vgui::IScheme *pScheme) final;
	void ResetSizes(const int wide, const int tall);
	void OnMessage(const KeyValues *params, vgui::VPANEL fromPanel) final;
	void FetchGameUIPanels();

	void Paint() final;
	void OnMainLoop(const NeoUI::Mode eMode);

	NeoUI::Context m_uiCtx;
	int m_iRowsInScreen = 0;
	int m_iRootSubPanelWide = 0;

	ConVar* m_pHostMap;

	bool m_bValidGameUIPanels = false;
	vgui::Frame *m_pLoadingPanel = nullptr;
	vgui::ProgressBar *m_pProgressBarMain = nullptr;
	vgui::Label *m_pLabelInfo = nullptr;

	enum ELoadingState
	{
		LOADINGSTATE_LOADING = 0,
		LOADINGSTATE_DISCONNECTED,

		LOADINGSTATE__TOTAL,
	};
	unsigned long m_aStrIdxMap[LOADINGSTATE__TOTAL] = {};
};
