#ifndef NEO_MATPROXY_THERMOPTIC_H
#define NEO_MATPROXY_THERMOPTIC_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterialproxy.h"

// forward declarations
void ToolFramework_RecordMaterialParams(IMaterial *pMaterial);

class CNEOTocMaterialProxy : public IMaterialProxy
{
public:
	CNEOTocMaterialProxy();
	virtual ~CNEOTocMaterialProxy();

	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);

	virtual void OnBind(void *pC_BaseEntity);
	virtual void Release() { delete this; }

	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_pResultVar;
};

#endif // NEO_MATPROXY_THERMOPTIC_H