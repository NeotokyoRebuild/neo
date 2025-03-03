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
    char m_NetworkedMessageKey[255];
    CNEOHud_Message* m_pHudMessage;
};
