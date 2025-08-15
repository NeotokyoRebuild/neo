#include "cbase.h"

enum ConvertType_t
{
    CONV_INVALID = -1,
    CONV_FLOAT,
    CONV_INTEGER,
    CONV_STRING,
};

class CNEO_TypeConverter : public CLogicalEntity
{
public:
    DECLARE_CLASS(CNEO_TypeConverter, CLogicalEntity);
    DECLARE_DATADESC();

    void InputSendData(inputdata_t& inputdata);

    ConvertType_t m_iOutputType = CONV_INVALID;

    COutputFloat   m_OutFloat;
    COutputInt     m_OutInteger;
    COutputString  m_OutString;
};

// An entity to convert with string conversion support to get around that pesky "Bad I/O link!!" error
LINK_ENTITY_TO_CLASS(neo_type_converter, CNEO_TypeConverter);

BEGIN_DATADESC(CNEO_TypeConverter)

DEFINE_KEYFIELD(m_iOutputType, FIELD_INTEGER, "outputtype"),

// Inputs
DEFINE_INPUTFUNC(FIELD_FLOAT, "SendFloat", InputSendData),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SendInteger", InputSendData),
DEFINE_INPUTFUNC(FIELD_STRING, "SendString", InputSendData),

// Outputs
DEFINE_OUTPUT(m_OutFloat, "OutFloat"),
DEFINE_OUTPUT(m_OutInteger, "OutInteger"),
DEFINE_OUTPUT(m_OutString, "OutString"),

END_DATADESC()

void CNEO_TypeConverter::InputSendData(inputdata_t& inputdata)
{
    switch (m_iOutputType)
    {
    case CONV_FLOAT:
        m_OutFloat.Set(inputdata.value.Float(), inputdata.pActivator, this);
        break;
    case CONV_INTEGER:
        m_OutInteger.Set(inputdata.value.Int(), inputdata.pActivator, this);
        break;
    case CONV_STRING:
        m_OutString.Set(AllocPooledString(inputdata.value.String()), inputdata.pActivator, this);
        break;
    default:
        AssertMsg(false, "Unexpected neo_type_converter output type: %d", m_iOutputType);
        break;
    }
}
