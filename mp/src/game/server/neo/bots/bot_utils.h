//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_UTILS_H
#define BOT_UTILS_H

#ifdef _WIN32
#pragma once
#endif

#ifdef INSOURCE_DLL
#include "in_shareddefs.h"
#endif

#include "bots\bot_defs.h"

#ifdef time
#undef time
#endif

class IBot;

//================================================================================
// Memory about an entity
//================================================================================
class CEntityMemory
{
public:
    DECLARE_CLASS_NOBASE( CEntityMemory );

    CEntityMemory( IBot *pBot, CBaseEntity *pEntity, CBaseEntity *pInformer = NULL );

    virtual CBaseEntity *GetEntity() const {
        return m_hEntity.Get();
    }

    virtual bool Is( CBaseEntity *pEntity ) const {
        return (GetEntity() == pEntity);
    }

    virtual CBaseEntity *GetInformer() const {
        return m_hInformer.Get();
    }

    virtual void SetInformer( CBaseEntity *pInformer ) {
        m_hInformer = pInformer;
    }

    virtual void UpdatePosition( const Vector &pos ) {
        m_vecLastPosition = pos;
        m_vecIdealPosition = pos;
        m_LastUpdate.Start();
    }

    // Visibility
    virtual bool IsVisible() const {
        return m_bVisible;
    }

    virtual void UpdateVisibility( bool visible ) {
        m_bVisible = visible;

        if ( visible ) {
            m_LastVisible.Start();
        }
    }

    virtual bool IsVisibleRecently( float seconds ) const {
        return m_LastVisible.IsLessThen( seconds );
    }

    virtual bool WasEverVisible() const {
        return m_LastVisible.HasStarted();
    }

    virtual float GetElapsedTimeSinceVisible() const {
        return m_LastVisible.GetElapsedTime();
    }

    virtual float GetTimeLastVisible() const {
        return m_LastVisible.GetStartTime();
    }

    // Position
    virtual const Vector GetLastKnownPosition() const {
        return m_vecLastPosition;
    }

    virtual const Vector GetIdealPosition() const {
        return m_vecIdealPosition;
    }

    virtual bool IsUpdatedRecently( float seconds ) const {
        return m_LastUpdate.IsLessThen( seconds );
    }

    virtual float GetElapsedTimeSinceUpdate() const {
        return m_LastUpdate.GetElapsedTime();
    }

    virtual float GetTimeLastUpdate() const {
        return m_LastUpdate.GetStartTime();
    }

    virtual float GetFrameLastUpdate() const {
        return m_flFrameLastUpdate;
    }

    virtual void MarkLastFrame() {
        m_flFrameLastUpdate = gpGlobals->absoluteframetime;
    }

    //
    virtual float GetTimeLeft();
    virtual bool IsLost();

    virtual const HitboxPositions GetHitbox() const {
        return m_Hitbox;
    }

    virtual const HitboxPositions GetVisibleHitbox() const {
        return m_VisibleHitbox;
    }

    virtual bool IsHitboxVisible( HitboxType part );

    virtual CNavArea *GetLastKnownArea() const {
        return TheNavMesh->GetNearestNavArea( m_vecLastPosition );
    }

    virtual float GetDistance() const;
    virtual float GetDistanceSquare() const;

    virtual bool IsInRange( float distance ) const {
        return (GetDistance() <= distance);
    }

    virtual bool IsEnemy() const;
    virtual bool IsFriend() const;

    virtual void Maintain() {
        UpdatePosition( GetLastKnownPosition() );
    }

    virtual bool GetVisibleHitboxPosition( Vector &vecPosition, HitboxType favorite );
    virtual void UpdateHitboxAndVisibility();

protected:
    EHANDLE m_hEntity;
    EHANDLE m_hInformer;

    IBot *m_pBot;

    Vector m_vecLastPosition;
    Vector m_vecIdealPosition;

    HitboxPositions m_Hitbox;
    HitboxPositions m_VisibleHitbox;

    bool m_bVisible;

    IntervalTimer m_LastVisible;
    IntervalTimer m_LastUpdate;

    float m_flFrameLastUpdate;
};

//================================================================================
// Memory about certain information
//================================================================================
class CDataMemory : public CMultidata
{
public:
    DECLARE_CLASS_GAMEROOT( CDataMemory, CMultidata );

    CDataMemory() : BaseClass()
    {
    }

    CDataMemory( int value ) : BaseClass( value )
    {
    }

    CDataMemory( Vector value ) : BaseClass( value )
    {
    }

    CDataMemory( float value ) : BaseClass( value )
    {
    }

    CDataMemory( const char *value ) : BaseClass( value )
    {
    }

    CDataMemory( CBaseEntity *value ) : BaseClass( value )
    {
    }

    virtual void Reset() {
        BaseClass::Reset();

        m_LastUpdate.Invalidate();
        m_flForget = -1.0f;
    }

    virtual void OnSet() {
        m_LastUpdate.Start();
    }

    bool IsExpired() {
        // Never expires
        if ( m_flForget <= 0.0f )
            return false;

        if ( !m_LastUpdate.HasStarted() )
            return false;

        return (m_LastUpdate.GetElapsedTime() >= m_flForget);
    }

    float GetElapsedTimeSinceUpdated() const {
        return m_LastUpdate.GetElapsedTime();
    }

    float GetTimeLastUpdate() const {
        return m_LastUpdate.GetStartTime();
    }

    void ForgetIn( float time ) {
        m_flForget = time;
    }

protected:
    IntervalTimer m_LastUpdate;
    float m_flForget;
};

//================================================================================
// Bot information
//================================================================================
class CBotProfile
{
public:
    CBotProfile();
    CBotProfile( int skill );

    virtual bool IsEasy() {
        return (GetSkill() == SKILL_EASY);
    }

    virtual bool IsMedium() {
        return (GetSkill() == SKILL_MEDIUM);
    }

    virtual bool IsHard() {
        return (GetSkill() == SKILL_HARD);
    }

    virtual bool IsVeryHard() {
        return (GetSkill() == SKILL_VERY_HARD);
    }

    virtual bool IsUltraHard() {
        return (GetSkill() == SKILL_ULTRA_HARD);
    }

    virtual bool IsRealistic() {
        return (GetSkill() == SKILL_IMPOSIBLE);
    }

    virtual bool IsEasiest() {
        return (GetSkill() == SKILL_EASIEST);
    }

    virtual bool IsHardest() {
        return (GetSkill() == SKILL_HARDEST);
    }

    virtual void SetSkill( int skill );

    virtual int GetSkill() {
        return m_iSkillLevel;
    }

    virtual const char *GeSkillName();

    virtual float GetMemoryDuration() {
        return m_flMemoryDuration;
    }

    virtual void SetMemoryDuration( float duration ) {
        m_flMemoryDuration = duration;
    }

    virtual int GetMinAimSpeed() {
        return m_iMinAimSpeed;
    }

    virtual int GetMaxAimSpeed() {
        return m_iMaxAimSpeed;
    }

    virtual void SetAimSpeed( int min, int max ) {
        m_iMinAimSpeed = min;
        m_iMaxAimSpeed = max;
    }

    virtual float GetAttackDelay() {
        return m_flAttackDelay;
    }

    virtual void SetAttackDelay( float time ) {
        m_flAttackDelay = time;
    }

    virtual HitboxType GetFavoriteHitbox() {
        return m_iFavoriteHitbox;
    }

    virtual void SetFavoriteHitbox( HitboxType type ) {
        m_iFavoriteHitbox = type;
    }

    virtual float GetAlertDuration() {
        return m_flAlertDuration;
    }

    virtual void SetAlertDuration( float duration ) {
        m_flAlertDuration = duration;
    }

    virtual float GetAggression() {
        return m_flAggression;
    }

    virtual void SetAggression( float value ) {
        m_flAggression = value;
    }

    virtual float GetReactionDelay() {
        return m_flReactionDelay;
    }

    virtual void SetReactionDelay( float delay ) {
        m_flReactionDelay = delay;
    }

protected:
    int m_iSkillLevel;
    float m_flMemoryDuration;
    float m_flAttackDelay;
    float m_flAlertDuration;
    float m_flAggression;
    float m_flReactionDelay;

    int m_iMinAimSpeed;
    int m_iMaxAimSpeed;

    HitboxType m_iFavoriteHitbox;
    
};

//================================================================================
// Información acerca de una tarea, se conforma de la tarea que se debe ejecutar
// y un valor que puede ser un Vector, un flotante, un string, etc.
//================================================================================
struct BotTaskInfo_t
{
    BotTaskInfo_t( int iTask )
    {
        task = iTask;

        vecValue.Invalidate();
        flValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, int value )
    {
        task = iTask;
        iValue = value;
        flValue = (float)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, bool value )
    {
        task = iTask;
        iValue = (value == true) ? 1 : 0;
        flValue = (float)iValue;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, Vector value )
    {
        task = iTask;
        vecValue = value;

        flValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, float value )
    {
        task = iTask;
        flValue = value;
        iValue = (int)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, const char *value )
    {
        task = iTask;
        iszValue = MAKE_STRING( value );

        vecValue.Invalidate();
        flValue = 0;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, CBaseEntity *value )
    {
        task = iTask;
        pszValue = value;

        vecValue.Invalidate();
        flValue = 0;
        iszValue = NULL_STRING;
    }

    int task;

    Vector vecValue;
    float flValue;
    int iValue;
    string_t iszValue;
    EHANDLE pszValue;
};

#endif // BOT_UTILS_H