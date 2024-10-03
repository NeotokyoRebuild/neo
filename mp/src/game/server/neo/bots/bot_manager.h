//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_MANAGER_H
#define BOT_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
// Sistema de bots
//================================================================================
class CBotManager : public CAutoGameSystemPerFrame
{
public:
    CBotManager();

    virtual bool Init();

    virtual void LevelInitPostEntity();
    virtual void LevelShutdownPreEntity();

    virtual void FrameUpdatePreEntityThink();
    virtual void FrameUpdatePostEntityThink();
};

extern CBotManager *TheBots;

#endif // BOT_MANAGER_H
