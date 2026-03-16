#include "c_neo_path_effect.h"
#include "neo_gamerules.h"
#include "cliententitylist.h"
#include "view.h"

CPathObjectManager* g_PathObjectManager = nullptr;

CPathObjectManager::CPathObjectManager()
{
	g_PathObjectManager = this;
}

void CPathObjectManager::RenderPaths()
{
	UpdatePaths();

	CMatRenderContextPtr pRenderContext(materials);
	IMesh* pMesh = pRenderContext->GetDynamicMesh();
	
	// Set override shader to the same simple shader we use to render the glow models
	IMaterial *pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
	g_pStudioRender->ForcedMaterialOverride( pMatGlowColor );

	Vector viewUp = CurrentViewUp();

	for (int pathIndex = 0; pathIndex < m_pathObjects.Size(); pathIndex++)
	{
		const int numPoints = m_pathObjects[pathIndex].m_pointsOnPath.Size();
		const int numLines = numPoints - 1;
		if (numLines <= 0)
			continue;

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, numLines );

		Vector previousPosition = m_pathObjects[pathIndex].m_pointsOnPath[0].m_vPositionInWorldSpace;
		for (int pointIndex = 1; pointIndex < numLines; ++pointIndex)
		{
			const float fractionOfPath = (float)pointIndex / numPoints;
			Vector4D vertexColor = m_pathObjects[pathIndex].m_startColor * (1 - fractionOfPath) + m_pathObjects[pathIndex].m_endColor * fractionOfPath;
			render->SetBlend( vertexColor[3] );
			render->SetColorModulation( &vertexColor[0]); // This only sets rgb, not alpha

			Vector position = m_pathObjects[pathIndex].m_pointsOnPath[pointIndex].m_vPositionInWorldSpace;
			Vector positionInBetween = previousPosition + (position - previousPosition) + viewUp;

			meshBuilder.Position3fv(previousPosition.Base());
			meshBuilder.Color4ub(vertexColor.x, vertexColor.y, vertexColor.z, vertexColor.w);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(positionInBetween.Base());
			meshBuilder.Color4ub(vertexColor.x, vertexColor.y, vertexColor.z, vertexColor.w);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(position.Base());
			meshBuilder.Color4ub(vertexColor.x, vertexColor.y, vertexColor.z, vertexColor.w);
			meshBuilder.AdvanceVertex();

			Vector nextPositionInBetween = position + (m_pathObjects[pathIndex].m_pointsOnPath[pointIndex+1].m_vPositionInWorldSpace - position) + viewUp;

			meshBuilder.Position3fv(position.Base());
			meshBuilder.Color4ub(vertexColor.x, vertexColor.y, vertexColor.z, vertexColor.w);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(nextPositionInBetween.Base());
			meshBuilder.Color4ub(vertexColor.x, vertexColor.y, vertexColor.z, vertexColor.w);
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv(positionInBetween.Base());
			meshBuilder.Color4ub(vertexColor.x, vertexColor.y, vertexColor.z, vertexColor.w);
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		pMesh->Draw();
	}

	g_pStudioRender->ForcedMaterialOverride( nullptr );
}

float nextUpdateTime;
void CPathObjectManager::UpdatePaths()
{
	for (int pathIndex = 0; pathIndex < m_pathObjects.Size(); pathIndex++)
	{
		/*if (C_BaseEntity* pEntity = ClientEntityList().GetEnt(m_pathObjects[pathIndex].m_pEntity);
			pEntity)
		{
			const Vector origin = pEntity->GetAbsOrigin();
			m_pathObjects[pathIndex].m_pointsOnPath.AddToTail(CPathObjectManager::PathObject_t::PointOnPath_t(origin, gpGlobals->curtime + m_pathObjects[pathIndex].m_flSegmentLifeTime));
		}else*/ if (m_pathObjects[pathIndex].m_hEntity.Get())
		{
			C_BaseEntity *pEntity = C_BaseEntity::Instance( m_pathObjects[pathIndex].m_hEntity );
			if (pEntity)
			{
				const Vector origin = pEntity->GetAbsOrigin();
				m_pathObjects[pathIndex].m_pointsOnPath.AddToTail(CPathObjectManager::PathObject_t::PointOnPath_t(origin, gpGlobals->curtime + m_pathObjects[pathIndex].m_flSegmentLifeTime));
			}
		}
		
		int numPointsToRemove = 0;
		for (int pointIndex = 0; pointIndex < m_pathObjects[pathIndex].m_pointsOnPath.Size(); ++pointIndex)
		{
			if (m_pathObjects[pathIndex].m_pointsOnPath[pointIndex].m_flDieTime > gpGlobals->curtime)
			{
				break;
			}
			numPointsToRemove++;
		}

		m_pathObjects[pathIndex].m_pointsOnPath.RemoveMultipleFromHead(numPointsToRemove);
	}
}
