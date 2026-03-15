#pragma once

#include "utlvector.h"
#include "mathlib/vector.h"

class C_BaseEntity;
class CViewSetup;
class CMatRenderContextPtr;

class CPathObjectManager
{
public:
	CPathObjectManager(){}

	int RegisterGlowObject( C_BaseEntity *pEntity, const Vector &vGlowColor, float flGlowAlpha, bool bRenderWhenOccluded, bool bRenderWhenUnoccluded, int nSplitScreenSlot )
	{
		int nIndex;
		if ( m_nFirstFreeSlot == GlowObjectDefinition_t::END_OF_FREE_LIST )
		{
			nIndex = m_GlowObjectDefinitions.AddToTail();
		}
		else
		{
			nIndex = m_nFirstFreeSlot;
			m_nFirstFreeSlot = m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot;
		}
		
		m_GlowObjectDefinitions[nIndex].m_hEntity = pEntity;
		m_GlowObjectDefinitions[nIndex].m_vGlowColor = vGlowColor;
		m_GlowObjectDefinitions[nIndex].m_flGlowAlpha = flGlowAlpha;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenOccluded = bRenderWhenOccluded;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenUnoccluded = bRenderWhenUnoccluded;
		m_GlowObjectDefinitions[nIndex].m_nSplitScreenSlot = nSplitScreenSlot;
		m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot = GlowObjectDefinition_t::ENTRY_IN_USE;

		return nIndex;
	}

	void UnregisterGlowObject( int nGlowObjectHandle )
	{
		Assert( !m_GlowObjectDefinitions[nGlowObjectHandle].IsUnused() );

		m_GlowObjectDefinitions[nGlowObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
		m_GlowObjectDefinitions[nGlowObjectHandle].m_hEntity = NULL;
		m_nFirstFreeSlot = nGlowObjectHandle;
	}

	void SetEntity( int nGlowObjectHandle, C_BaseEntity *pEntity )
	{
		Assert( !m_GlowObjectDefinitions[nGlowObjectHandle].IsUnused() );
		m_GlowObjectDefinitions[nGlowObjectHandle].m_hEntity = pEntity;
	}

	void SetColor( int index, const Color color )
	{
		m_pathObjects[index].m_startColor = m_pathObjects[index].m_endColor = color;
	}

	void SetStartColor( int index, const Color color ) 
	{
		m_pathObjects[index].m_startColor = color;
	}

	void SetEndColor( int index, const Color color ) 
	{
		m_pathObjects[index].m_endColor = color;
	}

	bool HasGlowEffect( C_BaseEntity *pEntity ) const
	{
		for ( int i = 0; i < m_GlowObjectDefinitions.Count(); ++ i )
		{
			if ( !m_GlowObjectDefinitions[i].IsUnused() && m_GlowObjectDefinitions[i].m_hEntity.Get() == pEntity )
			{
				return true;
			}
		}

		return false;
	}

	void RenderGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot );
private:

	void RenderPaths( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext );
	void ApplyEntityGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext, float flBloomScale, int x, int y, int w, int h );

	struct PathObject_t
	{
		void Draw();

		EHANDLE m_hEntity;
		Color m_startColor;
		Color m_endColor;
		float m_flFadeTime;
		bool m_bDepthTest = false;

		struct PointOnPath_t
		{
			Vector m_vPositionInWorldSpace;
			float m_flDieTime;
		};
		CUtlVector<PointOnPath_t> m_pointsOnPath;
	};
	CUtlVector<PathObject_t> m_pathObjects;
};

extern CPathManager g_PathManager;

class CGlowObject
{
public:
	CGlowObject( C_BaseEntity *pEntity, const Vector &vGlowColor = Vector( 1.0f, 1.0f, 1.0f ), float flGlowAlpha = 1.0f, bool bRenderWhenOccluded = false, bool bRenderWhenUnoccluded = false, int nSplitScreenSlot = GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS )
	{
		m_nGlowObjectHandle = g_GlowObjectManager.RegisterGlowObject( pEntity, vGlowColor, flGlowAlpha, bRenderWhenOccluded, bRenderWhenUnoccluded, nSplitScreenSlot );
	}

	~CGlowObject()
	{
		g_GlowObjectManager.UnregisterGlowObject( m_nGlowObjectHandle );
	}

	void SetEntity( C_BaseEntity *pEntity )
	{
		g_GlowObjectManager.SetEntity( m_nGlowObjectHandle, pEntity );
	}

	void SetColor( const Vector &vGlowColor )
	{
		g_GlowObjectManager.SetColor( m_nGlowObjectHandle, vGlowColor );
	}

	void SetAlpha( float flAlpha )
	{
		g_GlowObjectManager.SetAlpha( m_nGlowObjectHandle, flAlpha );
	}

	void SetRenderFlags( bool bRenderWhenOccluded, bool bRenderWhenUnoccluded )
	{
		g_GlowObjectManager.SetRenderFlags( m_nGlowObjectHandle, bRenderWhenOccluded, bRenderWhenUnoccluded );
	}

	bool IsRenderingWhenOccluded() const
	{
		return g_GlowObjectManager.IsRenderingWhenOccluded( m_nGlowObjectHandle );
	}

	bool IsRenderingWhenUnoccluded() const
	{
		return g_GlowObjectManager.IsRenderingWhenUnoccluded( m_nGlowObjectHandle );
	}

	bool IsRendering() const
	{
		return IsRenderingWhenOccluded() || IsRenderingWhenUnoccluded();
	}

	// Add more accessors/mutators here as needed

#ifdef NEO
	void SetUseTexturedHighlight(bool value)
	{
		g_GlowObjectManager.SetUseTexturedHighlight( m_nGlowObjectHandle, value );
	}
#endif // NEO

private:
	int m_nGlowObjectHandle;

	// Assignment & copy-construction disallowed
	CGlowObject( const CGlowObject &other );
	CGlowObject& operator=( const CGlowObject &other );
};