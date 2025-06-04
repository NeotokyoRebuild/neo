#pragma once

#include "ui/neo_ui.h"
#include <vgui_controls/EditablePanel.h>
#include <tier1/ilocalize.h>

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

	enum ELoadingState
	{
		LOADINGSTATE_LOADING = 0,
		LOADINGSTATE_DISCONNECTED,

		LOADINGSTATE__TOTAL,
	};

	struct RetGameUIPanels
	{
		vgui::Frame *pLoadingPanel; // If nullptr have no loading panels
		vgui::ProgressBar *pProgressBarMain;
		vgui::Label *pLabelInfo;
		StringIndex_t aStrIdxMap[LOADINGSTATE__TOTAL];
	};

	static RetGameUIPanels FetchGameUIPanels();

	void Paint() final;

	// NEO JANK (nullsystem): Keeping a pointer to the panels on an OnMessage then
	// using it later may not be reliable, so instead have an OnThink to refetch it
	// from time to time. Limited to only when g_pNeoRoot->m_bOnLoadingScreen and
	// at an interval.
	void OnThink();

	NeoUI::Context m_uiCtx;
	int m_iRowsInScreen = 0;
	int m_iRootSubPanelWide = 0;

	struct LoadingInfos
	{
		float flProgressBarMain;
		wchar_t wszTitle[256];
		wchar_t wszLabelInfo[1024];
		StringIndex_t aStrIdxMap[LOADINGSTATE__TOTAL];
		StringIndex_t iStrIdx;
	};
	LoadingInfos m_info = {};
};
