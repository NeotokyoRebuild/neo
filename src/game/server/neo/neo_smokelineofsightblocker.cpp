#include "cbase.h"
#include "util.h"
#include "neo_smokelineofsightblocker.h"

LINK_ENTITY_TO_CLASS(env_smokelineofsightblocker, CNEOSmokeLineOfSightBlocker);
BEGIN_DATADESC(CNEOSmokeLineOfSightBlocker)
    DEFINE_FIELD(m_mins, FIELD_VECTOR),
    DEFINE_FIELD(m_maxs, FIELD_VECTOR),
END_DATADESC()

