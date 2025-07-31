#include "baseentity.h"

class CNEO_Message : public CBaseEntity
{
    DECLARE_CLASS(CNEO_Message, CBaseEntity);
    DECLARE_SERVERCLASS();

public:
    DECLARE_DATADESC();

    virtual void Spawn() override;
    virtual void Precache() override;
    virtual void Think() override;
    virtual int UpdateTransmitState() override;

    CNetworkString(m_NetworkedMessageKey, 256);
    CNetworkString(m_NetworkedSubMessageKey, 256);
    CNetworkVar(bool, m_bTimerMode);

private:
    void InputShowMessage(inputdata_t& inputData);
    void InputHideMessage(inputdata_t& inputData);
    void InputShowSubMessage(inputdata_t& inputData);
    void InputHideSubMessage(inputdata_t& inputData);
    void InputStopTimer(inputdata_t& inputData);

    void DisplayTimer();

    string_t m_sSound;
    float m_SoundVolume;
    float m_flTimerStart;
};
