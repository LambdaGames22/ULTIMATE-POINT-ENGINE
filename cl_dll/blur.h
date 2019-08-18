/***
*
*	Copyright (c) 2016-2020, UPTeam. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This code is made by UPTeam (Half-Life: Ultimate Point developers)
*   Coders: Step4enko/LambdaGames22
*
*   https://www.moddb.com/mods/half-life-ultimate-point
*
****/

class CBlurTexture
{
public:
    CBlurTexture();
    void Init(int width, int height);
    void BindTexture(int width, int height);
    void DrawQuad(int width, int height,int of);
    void Draw(int width, int height);
    unsigned int g_texture;
    
    float alpha;
    float r,g,b;
    float of;
};

class CBlur
{
public:
    void InitScreen(void);
    void DrawBlur(void);
    int blur_pos;
    bool AnimateNextFrame(int desiredFrameRate);
    
    CBlurTexture m_pTextures;
    CBlurTexture m_pScreen;
};

extern CBlur gBlur;