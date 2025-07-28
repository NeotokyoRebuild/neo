#include "c_baseentity.h"

class CNEOHud_Message;

class C_NEO_Message : public C_BaseEntity
{
    DECLARE_CLASS(C_NEO_Message, C_BaseEntity);
    DECLARE_CLIENTCLASS();

public:
    C_NEO_Message();

    void OnDataChanged(DataUpdateType_t updateType) override;

private:
    char m_NetworkedMessageKey[256]{};
    char m_NetworkedSubMessageKey[256]{};
    bool m_bTimerMode{};
    CNEOHud_Message* m_pHudMessage;
    wchar_t *m_LocalizedPrefix;
};
