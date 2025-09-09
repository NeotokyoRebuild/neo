#include "cbase.h"
#include "neo_message.h"

LINK_ENTITY_TO_CLASS(neo_message, CNEO_Message);

IMPLEMENT_SERVERCLASS_ST(CNEO_Message, DT_NEO_Message)
	SendPropString(SENDINFO(m_NetworkedMessageKey)),
	SendPropString(SENDINFO(m_NetworkedSubMessageKey)),
	SendPropBool(SENDINFO(m_bTimerMode))
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Message)
	DEFINE_KEYFIELD(m_sSound, FIELD_SOUNDNAME, "sound"),
	DEFINE_KEYFIELD(m_SoundVolume, FIELD_FLOAT, "volume"),
	DEFINE_KEYFIELD(m_bTimerMode, FIELD_BOOLEAN, "timermode"),

// Inputs
	DEFINE_INPUTFUNC(FIELD_STRING, "ShowMessage", InputShowMessage),
	DEFINE_INPUTFUNC(FIELD_VOID, "HideMessage", InputHideMessage),
	DEFINE_INPUTFUNC(FIELD_STRING, "ShowSubMessage", InputShowSubMessage),
	DEFINE_INPUTFUNC(FIELD_VOID, "HideSubMessage", InputHideSubMessage),
	DEFINE_INPUTFUNC(FIELD_VOID, "StopTimer", InputStopTimer),

	DEFINE_THINKFUNC(Think)
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

	SetNextThink(TICK_NEVER_THINK);
}

void CNEO_Message::Precache()
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

void CNEO_Message::Think()
{
	DisplayTimer();
	SetNextThink(gpGlobals->curtime + 0.05f);
}

void CNEO_Message::DisplayTimer()
{
	float elapsed = gpGlobals->curtime - m_flTimerStart;
	V_snprintf(m_NetworkedMessageKey.GetForModify(), sizeof(m_NetworkedMessageKey), "%02d:%02d:%03d", int(elapsed / 60.0f), int(elapsed) % 60, int((elapsed - int(elapsed)) * 1000.0f)); // minutes, seconds, milliseconds
}

// Inputs

void CNEO_Message::InputShowMessage(inputdata_t& inputData)
{
	if (m_bTimerMode)
	{
		m_flTimerStart = gpGlobals->curtime;
		SetNextThink(gpGlobals->curtime);
	}
	else
	{
		V_strncpy(m_NetworkedMessageKey.GetForModify(), inputData.value.String(), sizeof(m_NetworkedMessageKey));
	}

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
	*m_NetworkedMessageKey.GetForModify() = 0;
	*m_NetworkedSubMessageKey.GetForModify() = 0;
	if (m_bTimerMode)
	{
		SetNextThink(TICK_NEVER_THINK);
	}
}

void CNEO_Message::InputShowSubMessage(inputdata_t& inputData)
{
	V_strncpy(m_NetworkedSubMessageKey.GetForModify(), inputData.value.String(), sizeof(m_NetworkedSubMessageKey));
}

void CNEO_Message::InputHideSubMessage(inputdata_t& inputData)
{
	*m_NetworkedSubMessageKey.GetForModify() = 0;
}

void CNEO_Message::InputStopTimer(inputdata_t& inputData)
{
	if (m_bTimerMode)
	{
		SetNextThink(TICK_NEVER_THINK);
		DisplayTimer(); // Display the correct time when it stops
	}
}
