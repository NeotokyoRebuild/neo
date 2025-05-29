#include "cbase.h"
#include "c_neo_message.h"
#include "hud.h"
#include "ui/neo_hud_message.h"
#include "vgui/ILocalize.h"

IMPLEMENT_CLIENTCLASS_DT(C_NEO_Message, DT_NEO_Message, CNEO_Message)
    RecvPropString(RECVINFO(m_NetworkedMessageKey))
END_RECV_TABLE()

C_NEO_Message::C_NEO_Message()
{
    // DG: Why doesn't GET_HUDELEMENT(CNEOHud_Message) work here
    CHudElement* pPanelName = gHUD.FindElement("neo_message");
    m_pHudMessage = static_cast<CNEOHud_Message*>(pPanelName);
}

void C_NEO_Message::OnDataChanged(DataUpdateType_t updateType)
{
    if (!m_pHudMessage)
    {
        DevWarning("C_NEO_Message cannot find CNEOHud_Message\n");
        return;
    }

    if (Q_strlen(STRING(m_NetworkedMessageKey)) > 0)
    {
        wchar_t* localizedText = g_pVGuiLocalize->Find(STRING(m_NetworkedMessageKey));

        if (localizedText)
        {
            m_pHudMessage->ShowMessage(localizedText);
        }
    }
    else
    {
        m_pHudMessage->HideMessage();
    }
}
