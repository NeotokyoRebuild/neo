#ifndef NEO_PREDICTED_VIEWMODEL_H
#define NEO_PREDICTED_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predicted_viewmodel.h"

#if defined( CLIENT_DLL )
#define CNEOPredictedViewModel C_NEOPredictedViewModel
#define CNEO_Player C_NEO_Player
#endif

class CNEO_Player;

class CNEOPredictedViewModel : public CPredictedViewModel
{
    DECLARE_CLASS(CNEOPredictedViewModel, CPredictedViewModel);
public:
    DECLARE_NETWORKCLASS();

    CNEOPredictedViewModel( void );
    virtual ~CNEOPredictedViewModel( void );

    virtual void CalcViewModelView(CBasePlayer *pOwner,
        const Vector& eyePosition, const QAngle& eyeAngles);

    void CalcLean(CNEO_Player *player);

private:
    Vector m_vecLeanDolly;
    QAngle m_angLeanAngle;
};

#endif // NEO_PREDICTED_VIEWMODEL_H