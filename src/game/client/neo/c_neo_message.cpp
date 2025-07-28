#include "cbase.h"
#include "c_neo_message.h"
#include "hud.h"
#include "ui/neo_hud_message.h"
#include "vgui/ILocalize.h"

IMPLEMENT_CLIENTCLASS_DT(C_NEO_Message, DT_NEO_Message, CNEO_Message)
    RecvPropString(RECVINFO(m_NetworkedMessageKey)),
    RecvPropString(RECVINFO(m_NetworkedSubMessageKey)),
    RecvPropBool(RECVINFO(m_bTimerMode))
END_RECV_TABLE()

C_NEO_Message::C_NEO_Message()
{
    m_pHudMessage = GET_NAMED_HUDELEMENT(CNEOHud_Message, neo_message);

    m_LocalizedPrefix = g_pVGuiLocalize->Find("Tutorial_TAC_Time");
}

void C_NEO_Message::OnDataChanged(DataUpdateType_t updateType)
{
    if (!m_pHudMessage)
    {
        DevWarning("C_NEO_Message cannot find CNEOHud_Message\n");
        return;
    }

    if (*m_NetworkedMessageKey)
    {
        if (m_bTimerMode)
        {
            char message[64];
            V_snprintf(message, sizeof(message), "%ls %s", m_LocalizedPrefix, m_NetworkedMessageKey);
            wchar_t messageUnicode[sizeof(message)];
            g_pVGuiLocalize->ConvertANSIToUnicode(message, messageUnicode, sizeof(messageUnicode));
            m_pHudMessage->ShowMessage(messageUnicode);
        }
        else
        {
            wchar_t* localizedText = g_pVGuiLocalize->Find(m_NetworkedMessageKey);

            if (localizedText)
            {
                m_pHudMessage->ShowMessage(localizedText);
            }
        }
    }
    else
    {
        m_pHudMessage->HideMessage();
    }

    if (*m_NetworkedSubMessageKey)
    {
        wchar_t* localizedText = g_pVGuiLocalize->Find(m_NetworkedSubMessageKey);

        if (localizedText)
        {
            m_pHudMessage->ShowSubMessage(localizedText);
        }
    }
    else
    {
        m_pHudMessage->HideSubMessage();
    }
}
