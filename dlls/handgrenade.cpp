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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


#define	HANDGRENADE_PRIMARY_VOLUME		450

enum handgrenade_e 
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW2,	// medium
	HANDGRENADE_THROW3,	// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};


LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade );


void CHandGrenade::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HANDGRENADE;

	if (w_model)
		SET_MODEL(ENT(pev), STRING(w_model));
	else
	    SET_MODEL(ENT(pev), "models/w_grenade.mdl");
	
	if (FStringNull(v_model))
		v_model = MAKE_STRING("models/v_grenade.mdl");

	if (FStringNull(p_model))
		p_model = MAKE_STRING("models/p_grenade.mdl");

#ifndef CLIENT_DLL
	if (pev->dmg == 0)
		pev->dmg = gSkillData.plrDmgHandGrenade;
#endif
	if ( FStringNull(m_iDefaultAmmo) && m_iDefaultAmmo == 0 )
	    m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit(); // Get ready to fall down.
}

void CHandGrenade::Precache( void )
{
	if (w_model)
		PRECACHE_MODEL((char*)STRING(w_model));
	else
	    PRECACHE_MODEL("models/w_grenade.mdl");

	PRECACHE_MODEL("models/v_grenade.mdl");

	PRECACHE_MODEL("models/p_grenade.mdl");

	m_iGrenadeSpoon = PRECACHE_MODEL ("models/w_spoon.mdl");
	m_iGrenadePin = PRECACHE_MODEL ("models/w_pin.mdl");
}

int CHandGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHandGrenade::Deploy( )
{
	m_flReleaseThrow = -1;

	return DefaultDeploy( "models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar" );
}

BOOL CHandGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		SendWeaponAnim( HANDGRENADE_HOLSTER );
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_HANDGRENADE);
		SetThink( &CHandGrenade::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHandGrenade::PrimaryAttack()
{
	if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CHandGrenade::SecondaryAttack( void )
{
	if ( !m_flStartDrop && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
	{
		m_flStartDrop = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		//EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/nfl.wav", 1.0, ATTN_NORM);
		//EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/nfl.wav", 1.0, ATTN_NORM);
	}
}

void CHandGrenade::WeaponIdle( void )
{
	if ( m_flReleaseThrow == 0 && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flStartThrow )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if ( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 4;
		if ( flVel > 500 )
			flVel = 500;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		if ( flVel < 500 )
			SendWeaponAnim( HANDGRENADE_THROW1 );
		else if ( flVel < 1000 )
			SendWeaponAnim( HANDGRENADE_THROW2 );
		else
			SendWeaponAnim( HANDGRENADE_THROW3 );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(70,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( pev->origin + gpGlobals->v_up * RANDOM_LONG(20,26) + gpGlobals->v_forward * RANDOM_LONG(18,22), vecShellVelocity, pev->angles.y, m_iGrenadeSpoon, TE_BOUNCE_SHELL); //eject shell jonny
		EjectBrass ( pev->origin + gpGlobals->v_up * RANDOM_LONG(20,29) + gpGlobals->v_forward * RANDOM_LONG(15,27), vecShellVelocity, pev->angles.y, m_iGrenadePin, TE_BOUNCE_SHELL); //eject shell jonny


		if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);// ensure that the animation can finish playing
		}
		return;
	}
	else if ( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
			SendWeaponAnim( HANDGRENADE_DRAW );
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
		}
		else 
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
	if ( m_flStartDrop )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle * 5.75;
		
		if ( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 1;
		if ( flVel > 500 )
			flVel = 500;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 1;
		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity * 1;


		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartDrop - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		//CGrenade::DropTimed( m_pPlayer->pev, vecSrc, vecThrow, time );
		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		if ( flVel < 500 )
			SendWeaponAnim( HANDGRENADE_THROW1 );
		else if ( flVel < 1000 )
			SendWeaponAnim( HANDGRENADE_THROW2 );
		else
			SendWeaponAnim( HANDGRENADE_THROW3 );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flStartDrop = 0;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(70,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( pev->origin + gpGlobals->v_up * RANDOM_LONG(25,26) + gpGlobals->v_forward * RANDOM_LONG(18,21), vecShellVelocity, pev->angles.y, m_iGrenadeSpoon, TE_BOUNCE_SHELL); //eject shell jonny
		EjectBrass ( pev->origin + gpGlobals->v_up * RANDOM_LONG(26,29) + gpGlobals->v_forward * RANDOM_LONG(19,23), vecShellVelocity, pev->angles.y, m_iGrenadePin, TE_BOUNCE_SHELL); //eject shell jonny

		if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		}
		return;
	}
	else if ( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;
		m_flStartDrop = 0;

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
			SendWeaponAnim( HANDGRENADE_DRAW );
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
		}
		else 
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}