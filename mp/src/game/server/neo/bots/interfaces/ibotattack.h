//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_ATTACK_H
#define IBOT_ATTACK_H

#pragma once

//================================================================================
// Attack component
// Everything related to attacking
// You can create a custom attack component using this interface.
//================================================================================
abstract_class IBotAttack : public IBotComponent
{
public:
    DECLARE_CLASS_GAMEROOT( IBotAttack, IBotComponent );

    IBotAttack( IBot *bot ) : BaseClass( bot )
    {
    }

    enum
    {
        RANGE_PRIMARY_ATTACK = 1,
        RANGE_SECONDARY_ATTACK,
        MELEE_PRIMARY_ATTACK,
        MELEE_SECONDARY_ATTACK
    };

public:
    virtual void FiregunAttack() = 0;
    virtual void MeleeWeaponAttack() = 0;
    //virtual void OnAttack( int type ) = 0;
};

#endif // IBOT_ATTACK_H