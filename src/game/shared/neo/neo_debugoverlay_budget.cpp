//========= NEO =========================================================//
//
// Purpose: Budgeting proxy for the engine's IVDebugOverlay interface.
//			See neo_debugoverlay_budget.h for why this exists.
//
//======================================================================//

#include "cbase.h"
#include "neo_debugoverlay_budget.h"
#include "engine/ivdebugoverlay.h"

#include <stdarg.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// A large map with bots drawing paths requests tens of thousands of live
// overlays, and the renderer's dynamic-mesh pool fails well before that number.
// This keeps the engine's list small enough to draw safely; the client and the
// server each apply it to what they emit.
#define NEO_DEBUGOVERLAY_BUDGET_DEFAULT "4000"

#ifdef CLIENT_DLL
static ConVar neo_debugoverlay_budget( "cl_debugoverlay_budget", NEO_DEBUGOVERLAY_BUDGET_DEFAULT, FCVAR_CHEAT,
	"Maximum number of debug overlays allowed to be alive at once (0 = unlimited). Guards the renderer against runaway debug visualisers." );
#else
static ConVar neo_debugoverlay_budget( "sv_debugoverlay_budget", NEO_DEBUGOVERLAY_BUDGET_DEFAULT, FCVAR_CHEAT,
	"Maximum number of debug overlays allowed to be alive at once (0 = unlimited). Guards the renderer against runaway debug visualisers such as nb_debug path with many bots." );
#endif

// NDebugOverlay's arrows draw six outline lines and then, when alpha is
// non-zero, six more filled triangles; its circles fan fifteen triangles behind
// sixteen lines. The fill conveys nothing the outline does not, so leaving it off
// roughly halves the cost of every arrow and filled circle.
#ifdef CLIENT_DLL
static ConVar neo_debugoverlay_fill( "cl_debugoverlay_fill", "0", FCVAR_CHEAT,
	"Draw filled triangle debug overlays (arrow fill, circle and sphere fans). Roughly doubles the cost of every arrow and filled circle; their outlines are drawn either way." );
#else
static ConVar neo_debugoverlay_fill( "sv_debugoverlay_fill", "0", FCVAR_CHEAT,
	"Draw filled triangle debug overlays (arrow fill, circle and sphere fans). Roughly doubles the cost of every arrow and filled circle; their outlines are drawn either way." );
#endif

// The engine's real interface, kept separately so the budget can read the
// engine's overlay list without depending on the proxy object below.
static IVDebugOverlay *s_pRealDebugOverlay = NULL;

// Count the engine's live overlays, giving up once nStopAt have been seen.
// Callers only need to know whether the list has reached a threshold, and the
// early-out keeps this proportional to that threshold rather than to however far
// a runaway visualiser has got.
static int NEO_CountEngineDebugOverlays( int nStopAt )
{
	if ( !s_pRealDebugOverlay )
		return 0;

	int nCount = 0;
	for ( OverlayText_t *pCur = s_pRealDebugOverlay->GetFirst();
		  pCur && nCount < nStopAt;
		  pCur = s_pRealDebugOverlay->GetNext( pCur ) )
	{
		++nCount;
	}

	return nCount;
}

//=============================================================================
// Overlay budget
//
// Budgets "overlays alive at once" rather than overlays per frame. An overlay is
// charged for its whole lifetime, which bounds the size of the engine's list
// directly and makes a long-lived overlay proportionally more expensive than a
// transient one without needing to know who asked for it.
//
// Accounting uses a timer wheel: reserving an overlay bumps the live count and
// schedules a decrement in the bucket it expires in. Everything is O(1).
//=============================================================================
class CNEODebugOverlayBudget
{
public:
	CNEODebugOverlayBudget( void )
	{
		memset( m_nExpiring, 0, sizeof( m_nExpiring ) );
		m_nLive = 0;
		m_nHead = 0;
		m_nDropped = 0;
		m_flHeadTime = 0.0f;
		m_flNextReportTime = 0.0f;
		m_nLastSyncFrame = -1;
		m_bStale = true;
	}

	// Reserve room for one overlay living flDuration seconds. False means we are
	// at budget and the caller should drop it.
	bool Reserve( float flDuration )
	{
		const int budget = neo_debugoverlay_budget.GetInt();
		if ( budget <= 0 )
		{
			// Budgeting is off, so we are no longer tracking and anything we
			// believe about the list stops being true. Rebuild if it comes back on.
			m_bStale = true;
			return true;
		}

		Advance();
		SyncToEngine( budget );

		if ( m_nLive >= budget )
		{
			++m_nDropped;

			// Refusals are the common path once a visualiser is over budget, so
			// keep it to one compare and only pay for a warning when it is due.
			if ( gpGlobals->curtime >= m_flNextReportTime )
			{
				Report();
			}
			return false;
		}

		// A duration of NDEBUG_PERSIST_TILL_NEXT_SERVER, or anything shorter than
		// a bucket, lives for one bucket. Anything past the end of the wheel is
		// charged as if it lived for the whole horizon, which still makes it far
		// more expensive than a per-tick overlay.
		int nBuckets = ( flDuration > 0.0f ) ? ( 1 + (int)( flDuration / BUCKET_SECONDS ) ) : 1;
		nBuckets = clamp( nBuckets, 1, NUM_BUCKETS - 1 );

		m_nExpiring[ ( m_nHead + nBuckets ) % NUM_BUCKETS ] += 1;
		++m_nLive;
		return true;
	}

	int GetLive( void )
	{
		Advance();
		return m_nLive;
	}

	void Reset( void )
	{
		memset( m_nExpiring, 0, sizeof( m_nExpiring ) );
		m_nLive = 0;
		m_nHead = 0;
		m_nDropped = 0;
		m_flHeadTime = gpGlobals->curtime;
		m_bStale = false;
	}

private:
	// Correct the running estimate against the engine's real list, at most once
	// per frame.
	//
	// The wheel only predicts expiry, and it under-counts in two ways. A duration
	// of NDEBUG_PERSIST_TILL_NEXT_SERVER is charged one bucket but the engine
	// keeps it until the next server update, which can be far longer. And the
	// engine's list is shared between the client and server DLLs, each of which
	// charges only what it emitted - so a server-side visualiser can fill the list
	// that the client's render pass has to draw.
	//
	// Reading the list is the only way to see either, so ground truth wins when it
	// is higher. Taking the max rather than assigning keeps the wheel's pending
	// decrements valid, and means this can refuse work but never wave extra
	// through.
	void SyncToEngine( int budget )
	{
		if ( !s_pRealDebugOverlay || m_nLastSyncFrame == gpGlobals->framecount )
			return;

		m_nLastSyncFrame = gpGlobals->framecount;

		const int nActual = NEO_CountEngineDebugOverlays( budget + 1 );
		if ( nActual > m_nLive )
		{
			m_nLive = nActual;
		}
	}

	void Advance( void )
	{
		const float now = gpGlobals->curtime;

		// A level change or rewound clock invalidates the wheel, and the engine
		// discards its overlays at the same time. So does any stretch where we were
		// not tracking. Start over rather than locking the budget out on stale
		// bookkeeping for a whole horizon.
		if ( m_bStale || now < m_flHeadTime || ( now - m_flHeadTime ) > ( NUM_BUCKETS * BUCKET_SECONDS ) )
		{
			Reset();
			return;
		}

		while ( ( now - m_flHeadTime ) >= BUCKET_SECONDS )
		{
			m_flHeadTime += BUCKET_SECONDS;
			m_nHead = ( m_nHead + 1 ) % NUM_BUCKETS;
			m_nLive -= m_nExpiring[ m_nHead ];
			m_nExpiring[ m_nHead ] = 0;
		}

		if ( m_nLive < 0 )
		{
			m_nLive = 0;
		}
	}

	void Report( void )
	{
		m_flNextReportTime = gpGlobals->curtime + 5.0f;
		DevWarning( "%s reached (%d overlays alive, %d dropped in the last 5s). Debug overlays are being skipped.\n",
					neo_debugoverlay_budget.GetName(), m_nLive, m_nDropped );
		m_nDropped = 0;
	}

	// 512 buckets of 0.1s gives a ~51 second horizon for a couple of KB.
	enum { NUM_BUCKETS = 512 };
	static const float BUCKET_SECONDS;

	int		m_nExpiring[ NUM_BUCKETS ];	// overlays expiring in each bucket
	int		m_nLive;					// running total of the above
	int		m_nHead;					// bucket "now" lives in
	int		m_nDropped;					// overlays refused since the last report
	float	m_flHeadTime;				// curtime that m_nHead started at
	float	m_flNextReportTime;
	int		m_nLastSyncFrame;			// frame we last reconciled against the engine's list
	bool	m_bStale;					// bookkeeping is untrustworthy; rebuild on next use
};

const float CNEODebugOverlayBudget::BUCKET_SECONDS = 0.1f;

static CNEODebugOverlayBudget s_NEOOverlayBudget;


//=============================================================================
// The proxy
//
// Forwards every call to the real interface. Methods that add a persistent
// overlay take a budget slot first and drop the overlay when there is none;
// queries, iteration and screen-space text pass straight through.
// ClearAllOverlays() also forgets the budget, which covers both the
// clear_debug_overlays command and the VScript DebugDrawClear() path.
//
// The engine's text methods are printf-style varargs with no va_list variant, so
// those are formatted here and handed on as "%s".
//=============================================================================
class CNEODebugOverlayProxy : public IVDebugOverlay
{
public:
	CNEODebugOverlayProxy( void ) : m_pReal( NULL ) {}

	void SetReal( IVDebugOverlay *pReal ) { m_pReal = pReal; }

	virtual void AddEntityTextOverlay( int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *format, ... ) OVERRIDE
	{
		if ( !s_NEOOverlayBudget.Reserve( duration ) )
			return;

		char szBuf[ 1024 ];
		va_list args;
		va_start( args, format );
		V_vsnprintf( szBuf, sizeof( szBuf ), format, args );
		va_end( args );

		m_pReal->AddEntityTextOverlay( ent_index, line_offset, duration, r, g, b, a, "%s", szBuf );
	}

	virtual void AddBoxOverlay( const Vector &origin, const Vector &mins, const Vector &maxs, QAngle const &orientation, int r, int g, int b, int a, float duration ) OVERRIDE
	{
		if ( s_NEOOverlayBudget.Reserve( duration ) )
			m_pReal->AddBoxOverlay( origin, mins, maxs, orientation, r, g, b, a, duration );
	}

	virtual void AddTriangleOverlay( const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration ) OVERRIDE
	{
		// Solid overlay geometry is fill behind an outline that was already drawn
		// as lines, so dropping it is nearly free. See the convar above.
		if ( !neo_debugoverlay_fill.GetBool() )
			return;

		if ( s_NEOOverlayBudget.Reserve( duration ) )
			m_pReal->AddTriangleOverlay( p1, p2, p3, r, g, b, a, noDepthTest, duration );
	}

	virtual void AddLineOverlay( const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest, float duration ) OVERRIDE
	{
		if ( s_NEOOverlayBudget.Reserve( duration ) )
			m_pReal->AddLineOverlay( origin, dest, r, g, b, noDepthTest, duration );
	}

	virtual void AddTextOverlay( const Vector &origin, float duration, const char *format, ... ) OVERRIDE
	{
		if ( !s_NEOOverlayBudget.Reserve( duration ) )
			return;

		char szBuf[ 1024 ];
		va_list args;
		va_start( args, format );
		V_vsnprintf( szBuf, sizeof( szBuf ), format, args );
		va_end( args );

		m_pReal->AddTextOverlay( origin, duration, "%s", szBuf );
	}

	virtual void AddTextOverlay( const Vector &origin, int line_offset, float duration, const char *format, ... ) OVERRIDE
	{
		if ( !s_NEOOverlayBudget.Reserve( duration ) )
			return;

		char szBuf[ 1024 ];
		va_list args;
		va_start( args, format );
		V_vsnprintf( szBuf, sizeof( szBuf ), format, args );
		va_end( args );

		m_pReal->AddTextOverlay( origin, line_offset, duration, "%s", szBuf );
	}

	virtual void AddScreenTextOverlay( float flXPos, float flYPos, float flDuration, int r, int g, int b, int a, const char *text ) OVERRIDE
	{
		// Screen-space HUD text, not part of the world overlay list.
		m_pReal->AddScreenTextOverlay( flXPos, flYPos, flDuration, r, g, b, a, text );
	}

	virtual void AddSweptBoxOverlay( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float flDuration ) OVERRIDE
	{
		if ( s_NEOOverlayBudget.Reserve( flDuration ) )
			m_pReal->AddSweptBoxOverlay( start, end, mins, maxs, angles, r, g, b, a, flDuration );
	}

	virtual void AddGridOverlay( const Vector &origin ) OVERRIDE
	{
		m_pReal->AddGridOverlay( origin );
	}

	virtual int ScreenPosition( const Vector &point, Vector &screen ) OVERRIDE
	{
		return m_pReal->ScreenPosition( point, screen );
	}

	virtual int ScreenPosition( float flXPos, float flYPos, Vector &screen ) OVERRIDE
	{
		return m_pReal->ScreenPosition( flXPos, flYPos, screen );
	}

	virtual OverlayText_t *GetFirst( void ) OVERRIDE
	{
		return m_pReal->GetFirst();
	}

	virtual OverlayText_t *GetNext( OverlayText_t *current ) OVERRIDE
	{
		return m_pReal->GetNext( current );
	}

	virtual void ClearDeadOverlays( void ) OVERRIDE
	{
		m_pReal->ClearDeadOverlays();
	}

	virtual void ClearAllOverlays() OVERRIDE
	{
		// Nothing is alive any more, so the budget should not still hold room for it.
		s_NEOOverlayBudget.Reset();
		m_pReal->ClearAllOverlays();
	}

	virtual void AddTextOverlayRGB( const Vector &origin, int line_offset, float duration, float r, float g, float b, float alpha, const char *format, ... ) OVERRIDE
	{
		if ( !s_NEOOverlayBudget.Reserve( duration ) )
			return;

		char szBuf[ 1024 ];
		va_list args;
		va_start( args, format );
		V_vsnprintf( szBuf, sizeof( szBuf ), format, args );
		va_end( args );

		m_pReal->AddTextOverlayRGB( origin, line_offset, duration, r, g, b, alpha, "%s", szBuf );
	}

	virtual void AddTextOverlayRGB( const Vector &origin, int line_offset, float duration, int r, int g, int b, int a, const char *format, ... ) OVERRIDE
	{
		if ( !s_NEOOverlayBudget.Reserve( duration ) )
			return;

		char szBuf[ 1024 ];
		va_list args;
		va_start( args, format );
		V_vsnprintf( szBuf, sizeof( szBuf ), format, args );
		va_end( args );

		m_pReal->AddTextOverlayRGB( origin, line_offset, duration, r, g, b, a, "%s", szBuf );
	}

	virtual void AddLineOverlayAlpha( const Vector &origin, const Vector &dest, int r, int g, int b, int a, bool noDepthTest, float duration ) OVERRIDE
	{
		if ( s_NEOOverlayBudget.Reserve( duration ) )
			m_pReal->AddLineOverlayAlpha( origin, dest, r, g, b, a, noDepthTest, duration );
	}

	virtual void AddBoxOverlay2( const Vector &origin, const Vector &mins, const Vector &maxs, QAngle const &orientation, const Color &faceColor, const Color &edgeColor, float duration ) OVERRIDE
	{
		if ( s_NEOOverlayBudget.Reserve( duration ) )
			m_pReal->AddBoxOverlay2( origin, mins, maxs, orientation, faceColor, edgeColor, duration );
	}

	virtual void AddScreenTextOverlay2( float flXPos, float flYPos, int iLine, float flDuration, int r, int g, int b, int a, const char *text ) OVERRIDE
	{
		m_pReal->AddScreenTextOverlay2( flXPos, flYPos, iLine, flDuration, r, g, b, a, text );
	}

private:
	IVDebugOverlay *m_pReal;
};

static CNEODebugOverlayProxy s_NEODebugOverlayProxy;

IVDebugOverlay *NEO_InstallDebugOverlayBudget( IVDebugOverlay *pReal )
{
	if ( !pReal )
	{
		// A dedicated server has no debug overlay interface. Leave it NULL so the
		// "if ( debugoverlay )" guards in game code keep short-circuiting.
		return NULL;
	}

	s_pRealDebugOverlay = pReal;
	s_NEODebugOverlayProxy.SetReal( pReal );
	return &s_NEODebugOverlayProxy;
}


//-----------------------------------------------------------------------------
// Reports two numbers, because they answer different questions: what this DLL
// emitted is what its budget is enforcing, while what the engine holds is what
// the renderer has to draw every frame. On a listen server the latter includes
// the other DLL's overlays, so it is the one to watch when a visualiser starts
// making the game unstable.
//-----------------------------------------------------------------------------
static void NEO_CC_DebugOverlayReport( void )
{
	// Informative well past any sane budget, while still bounding the walk.
	const int nWalkLimit = 1000000;
	const int nEngineCount = NEO_CountEngineDebugOverlays( nWalkLimit );

	Msg( "debug overlays: this DLL emitted %d (budget %d, %s), engine list holds %d%s\n",
		 s_NEOOverlayBudget.GetLive(),
		 neo_debugoverlay_budget.GetInt(),
		 neo_debugoverlay_budget.GetName(),
		 nEngineCount,
		 ( nEngineCount >= nWalkLimit ) ? "+" : "" );
}

#ifdef CLIENT_DLL
static ConCommand neo_debugoverlay_report( "cl_debugoverlay_report", NEO_CC_DebugOverlayReport,
	"Report how many debug overlays the client emitted, and how many the engine's shared list holds.", FCVAR_CHEAT );
#else
static ConCommand neo_debugoverlay_report( "sv_debugoverlay_report", NEO_CC_DebugOverlayReport,
	"Report how many debug overlays the server emitted, and how many the engine's shared list holds.", FCVAR_CHEAT );
#endif
