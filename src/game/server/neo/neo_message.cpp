#include "cbase.h"
#include "neo_message.h"

LINK_ENTITY_TO_CLASS(neo_message, CNEO_Message);

IMPLEMENT_SERVERCLASS_ST(CNEO_Message, DT_NEO_Message)
    SendPropString(SENDINFO(m_NetworkedMessageKey))
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Message)
	DEFINE_KEYFIELD(m_sSound, FIELD_SOUNDNAME, "sound"),
	DEFINE_KEYFIELD(m_SoundVolume, FIELD_FLOAT, "volume"),

// Inputs
    DEFINE_INPUTFUNC(FIELD_STRING, "ShowMessage", InputShowMessage),
    DEFINE_INPUTFUNC(FIELD_VOID, "HideMessage", InputHideMessage)
END_DATADESC()

void CNEO_Message::Spawn()
{
    memset(m_NetworkedMessageKey.GetForModify(), 0, sizeof(m_NetworkedMessageKey));

    BaseClass::Spawn();
	Precache();

	m_SoundVolume *= 0.1;
	if (m_SoundVolume <= 0)
	{
		m_SoundVolume = 1.0;
	}
}

void CNEO_Message::Precache(void)
{
	if (m_sSound != NULL_STRING)
	{
		PrecacheScriptSound(STRING(m_sSound));
	}
}

int CNEO_Message::UpdateTransmitState()
{
    return SetTransmitState(FL_EDICT_ALWAYS);
}

// Inputs

void CNEO_Message::InputShowMessage(inputdata_t& inputData)
{
    const char* szMessageKey = inputData.value.String();
    Q_strncpy(m_NetworkedMessageKey.GetForModify(), szMessageKey, 255);

	if (m_sSound != NULL_STRING)
	{
		CRecipientFilter filter;
		filter.AddAllPlayers();

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = (char*)STRING(m_sSound);
		ep.m_flVolume = m_SoundVolume;
		ep.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_NONE);

		EmitSound(filter, entindex(), ep);
	}
}

void CNEO_Message::InputHideMessage(inputdata_t& inputData)
{
    Q_strncpy(m_NetworkedMessageKey.GetForModify(), "", 255);
}
