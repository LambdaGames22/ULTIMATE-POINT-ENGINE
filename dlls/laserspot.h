/***
*
*	Copyright (c) 2016-2020, EETeam. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This code is made by EETeam (Half-Life: Ultimate Point developers)
*   Coders: Step4enko / LambdaGames22
*
*   https://www.moddb.com/mods/half-life-ultimate-point
*
****/

class CLaserSpot2 : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );

	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }

public:
	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );
	
	static CLaserSpot2 *CreateSpot2( void );
};

CLaserSpot2 *m_pSpot;