//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VISMAT_H
#define VISMAT_H
#ifdef _WIN32
#pragma once
#endif



void BuildVisLeafs( int threadnum );


struct transfer_t;
transfer_t* BuildVisLeafs_Start();

void BuildVisLeafs_Cluster( int threadnum, transfer_t *transfers, int iCluster );

void BuildVisLeafs_End( transfer_t *transfers );



#endif // VISMAT_H
