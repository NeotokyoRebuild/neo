#include "baseentity.h"

class CNEO_Message : public CBaseEntity
{
    DECLARE_CLASS(CNEO_Message, CBaseEntity);
    DECLARE_SERVERCLASS();

public:
    DECLARE_DATADESC();

    void Spawn() override;
    void Precache(void);
    int UpdateTransmitState();

    CNetworkString(m_NetworkedMessageKey, 255);

private:
    void InputShowMessage(inputdata_t& inputData);
    void InputHideMessage(inputdata_t& inputData);

    string_t m_sSound;
    float m_SoundVolume;
};
