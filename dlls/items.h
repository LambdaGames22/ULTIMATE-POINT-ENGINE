/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef ITEMS_H
#define ITEMS_H


class CItem : public CBaseEntity
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd ); // Step4enko

	CBaseEntity*	Respawn( void );
	void	EXPORT ItemTouch( CBaseEntity *pOther );
	void	EXPORT Materialize( void );
	virtual BOOL MyTouch( CBasePlayer *pPlayer ) { return FALSE; };

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	int	    m_IszSound; // Step4enko

	string_t	m_iszCustomHudIcon; // Step4enko
};

#endif // ITEMS_H
