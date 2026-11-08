#pragma once

#include "NextBotVisionInterface.h"

//----------------------------------------------------------------------------
class CNEOBotVision : public IVision
{
public:
	CNEOBotVision( INextBot *bot ) : IVision( bot )
	{
	}

	virtual ~CNEOBotVision() { }

	/**
	 * Populate "potentiallyVisible" with the set of all entities we could potentially see. 
	 * Entities in this set will be tested for visibility/recognition in IVision::Update()
	 */
	virtual void CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible ) override;

	virtual bool IsIgnored( CBaseEntity *subject ) const override;		// return true to completely ignore this entity (may not be in sight when this is called)

	virtual float GetMaxVisionRange( void ) const override;				// return maximum distance vision can reach
	virtual float GetMinRecognizeTime( void ) const override;			// return VISUAL reaction time

	bool IsInFieldOfView( CBaseEntity *subject ) const override;

	bool IsAbleToSee(CBaseEntity *subject, FieldOfViewCheckType checkFOV, Vector *visibleSpot = nullptr) const override;
	// NEO NOTE (nullsystem): Not overriding non-CBaseEntity version as that doesn't give us enough info properly

	// TEMP: invalidate telemetry when existing vision operations forget its selection.
	void Reset() override
	{
		IVision::Reset();
		m_hTransitionTarget = nullptr;
	}

	void ForgetEntity( CBaseEntity *forgetMe ) override
	{
		IVision::ForgetEntity( forgetMe );
		if ( m_hTransitionTarget.Get() == forgetMe )
		{
			m_hTransitionTarget = nullptr;
		}
	}

	void ForgetAllKnownEntities() override
	{
		IVision::ForgetAllKnownEntities();
		m_hTransitionTarget = nullptr;
	}

	// TEMP: remember existing selections without running threat arbitration for telemetry.
	const CKnownEntity *GetPrimaryKnownThreat( bool onlyVisibleThreats = false ) const override
	{
		const CKnownEntity *threat = IVision::GetPrimaryKnownThreat( onlyVisibleThreats );
		m_hTransitionTarget = threat ? threat->GetEntity() : nullptr;
		return threat;
	}

	CBaseEntity *GetTransitionTarget() const { return m_hTransitionTarget.Get(); }

private:
	mutable CHandle< CBaseEntity > m_hTransitionTarget;
	CUtlVector< CHandle< CBaseCombatCharacter > > m_potentiallyVisibleNPCVector;
	CountdownTimer m_potentiallyVisibleUpdateTimer;
	void UpdatePotentiallyVisibleNPCVector( void );

	CountdownTimer m_scanTimer;
};
