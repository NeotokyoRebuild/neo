#include "cbase.h"

enum ConvertType_t
{
    CONV_FLOAT = 0,
    CONV_INTEGER,
    CONV_STRING,
};

class CNEO_TypeConverter : public CLogicalEntity
{
public:
    DECLARE_CLASS(CNEO_TypeConverter, CLogicalEntity);
    DECLARE_DATADESC();

    void InputSendFloat(inputdata_t& inputdata);
    void InputSendInteger(inputdata_t& inputdata);
    void InputSendString(inputdata_t& inputdata);

private:

    int m_iOutputType = 0;

    COutputFloat   m_OutFloat;
    COutputInt     m_OutInteger;
    COutputString  m_OutString;
};

// An entity to convert between variable types to get around that pesky "Bad I/O link!!" error

LINK_ENTITY_TO_CLASS(neo_type_converter, CNEO_TypeConverter);

BEGIN_DATADESC(CNEO_TypeConverter)

    DEFINE_KEYFIELD(m_iOutputType, FIELD_INTEGER, "outputtype"),

    // Inputs
    DEFINE_INPUTFUNC(FIELD_FLOAT, "SendFloat", InputSendFloat),
    DEFINE_INPUTFUNC(FIELD_INTEGER, "SendInteger", InputSendInteger),
    DEFINE_INPUTFUNC(FIELD_STRING, "SendString", InputSendString),

    // Outputs
    DEFINE_OUTPUT(m_OutFloat, "OutFloat"),
    DEFINE_OUTPUT(m_OutInteger, "OutInteger"),
    DEFINE_OUTPUT(m_OutString, "OutString"),

END_DATADESC()

void CNEO_TypeConverter::InputSendFloat(inputdata_t& inputdata)
{
    float f = inputdata.value.Float();
    switch (m_iOutputType)
    {
    case CONV_FLOAT:
    {
        m_OutFloat.Set(f, inputdata.pActivator, this);
        break;
    }
    case CONV_INTEGER:
    {
        m_OutInteger.Set((int)f, inputdata.pActivator, this);
        break;
    }
    case CONV_STRING:
    {
        char buffer[64];
        Q_snprintf(buffer, sizeof(buffer), "%f", f);
        string_t str = AllocPooledString(buffer);
        m_OutString.Set(str, inputdata.pActivator, this);
        break;
    }
    }
}

void CNEO_TypeConverter::InputSendInteger(inputdata_t& inputdata)
{
    int i = inputdata.value.Int();
    switch (m_iOutputType)
    {
    case CONV_FLOAT:
    {
        m_OutFloat.Set((float)i, inputdata.pActivator, this);
        break;
    }
    case CONV_INTEGER:
    {
        m_OutInteger.Set(i, inputdata.pActivator, this);
        break;
    }
    case CONV_STRING:
    {
        char buffer[64];
        Q_snprintf(buffer, sizeof(buffer), "%d", i);
        string_t str = AllocPooledString(buffer);
        m_OutString.Set(str, inputdata.pActivator, this);
        break;
    }
    }
}

void CNEO_TypeConverter::InputSendString(inputdata_t& inputdata)
{
    const char* s = inputdata.value.String();
    switch (m_iOutputType)
    {
    case CONV_FLOAT:
    {
        float f = atof(s);
        m_OutFloat.Set(f, inputdata.pActivator, this);
        break;
    }
    case CONV_INTEGER:
    {
        int i = atoi(s);
        m_OutInteger.Set(i, inputdata.pActivator, this);
        break;
    }
    case CONV_STRING:
    {
        string_t str = AllocPooledString(s);
        m_OutString.Set(str, inputdata.pActivator, this);
        break;
    }
    }
}