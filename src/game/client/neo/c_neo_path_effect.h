#pragma once

class C_BaseEntity;
class CViewSetup;
class CMatRenderContextPtr;

class CPathObjectManager
{
public:
	CPathObjectManager();

	int RegisterPathObject(  C_BaseEntity *pEntity, const Vector4D startColor, const Vector4D endColor, float flLifeTime, float flSegmentLifeTime, bool bDepthTest = false )
	{
		if (HasPathEffect(pEntity))
		{
			return -1;
		}

		const int index = m_pathObjects.AddToTail();
		m_pathObjects[index].m_hEntity.Set(pEntity);
		m_pathObjects[index].m_pEntity = pEntity->entindex();
		m_pathObjects[index].m_startColor = startColor;
		m_pathObjects[index].m_endColor = endColor;
		m_pathObjects[index].m_flLifeTime = flLifeTime;
		m_pathObjects[index].m_flSegmentLifeTime = flSegmentLifeTime;
		m_pathObjects[index].m_bDepthTest = bDepthTest;
		return index;
	}

	void SetColor( int index, const Vector4D color )
	{
		m_pathObjects[index].m_startColor = m_pathObjects[index].m_endColor = color;
	}

	void SetStartColor( int index, const Vector4D color ) 
	{
		m_pathObjects[index].m_startColor = color;
	}

	void SetEndColor( int index, const Vector4D color ) 
	{
		m_pathObjects[index].m_endColor = color;
	}

	bool HasPathEffect( C_BaseEntity *pEntity ) const
	{
		for ( int i = 0; i < m_pathObjects.Size(); ++ i )
		{
			if ( m_pathObjects[i].m_hEntity == pEntity )
				return true;
		}
		return false;
	}

	void RenderPaths();
private:

	void UpdatePaths();

	struct PathObject_t
	{
		void Draw();

		EHANDLE m_hEntity;
		int m_pEntity;
		Vector4D m_startColor;
		Vector4D m_endColor;
		float m_flLifeTime = 0.f;
		float m_flSegmentLifeTime = 0.f;
		float m_flLastUpdateTime = 0.f;
		bool m_bDepthTest = false;
		
		struct PointOnPath_t
		{
			Vector m_vPositionInWorldSpace;
			float m_flDieTime = 0.f;
		};
		CUtlVector<PointOnPath_t> m_pointsOnPath;
	};

	CUtlVector<PathObject_t> m_pathObjects;
};

extern CPathObjectManager* g_PathObjectManager;