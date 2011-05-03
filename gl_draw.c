/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2005-2007, 2009-2011 Mark Olsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "quakedef.h"
#include "gl_local.h"
#include "gl_state.h"
#include "version.h"
#include "sbar.h"
#include "wad.h"

#include "image.h"
#include "utils.h"
#include "config.h"

#include "draw.h"

struct Picture
{
	int texnum;
	float invwidth;
	float invheight;
};

static unsigned char drawgl_inited;

extern cvar_t scr_menualpha;

extern cvar_t crosshair, cl_crossx, cl_crossy, crosshaircolor, crosshairsize;
extern cvar_t scr_coloredText;


qboolean OnChange_gl_crosshairimage(cvar_t *, char *);
cvar_t	gl_crosshairimage   = {"crosshairimage", "", 0, OnChange_gl_crosshairimage};

qboolean OnChange_gl_consolefont (cvar_t *, char *);
cvar_t	gl_consolefont		= {"gl_consolefont", "original", 0, OnChange_gl_consolefont};

cvar_t	gl_crosshairalpha	= {"crosshairalpha", "1"};


qboolean OnChange_gl_smoothfont (cvar_t *var, char *string);
cvar_t gl_smoothfont = {"gl_smoothfont", "0", 0, OnChange_gl_smoothfont};

byte			*draw_chars;						// 8*8 graphic characters

static int		translate_texture;
static int		char_texture;


#define		NUMCROSSHAIRS 6
int			crosshairtextures[NUMCROSSHAIRS];
int			crosshairtexture_txt;
struct Picture *crosshairpic;

static byte customcrosshairdata[64];

#define CROSSHAIR_NONE	0
#define CROSSHAIR_TXT	1
#define CROSSHAIR_IMAGE	2
static int customcrosshair_loaded = CROSSHAIR_NONE;


static byte crosshairdata[NUMCROSSHAIRS][64] =
{
	{
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff,
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	},

	{
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	},

	{
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	},

	{
		0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
		0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
		0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff,
		0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
		0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
	},

	{
		0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 
		0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xff, 
		0xfe, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xfe, 0xff, 
		0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xff, 
		0xfe, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xfe, 0xff, 
		0xff, 0xfe, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 
		0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	},
	
	{
		0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 
		0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 
		0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	},
};

static void SetupFontSmoothing()
{
	if (!char_texture)
		return;

	GL_Bind(char_texture);

	if (gl_smoothfont.value)
	{
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
}

qboolean OnChange_gl_smoothfont (cvar_t *var, char *string)
{
	if (drawgl_inited)
	{
		var->value = Q_atof(string);

		SetupFontSmoothing();
	}

	return false;
}

static void Crosshair_LoadImage(const char *s)
{
	struct Picture *pic;

	if (!s[0])
	{
		customcrosshair_loaded &= ~CROSSHAIR_IMAGE;
		return;
	}

	if (*s)
	{
		if (crosshairpic)
		{
			Draw_FreePicture(crosshairpic);
			crosshairpic = 0;
		}

		pic = Draw_LoadPicture(va("crosshairs/%s", s), DRAW_LOADPICTURE_NOFALLBACK);
		if (pic)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			crosshairpic = pic;
			customcrosshair_loaded |= CROSSHAIR_IMAGE;

			return;
		}

		Com_Printf("Couldn't load image %s\n", s);
	}


	customcrosshair_loaded &= ~CROSSHAIR_IMAGE;
}

qboolean OnChange_gl_crosshairimage(cvar_t *v, char *s)
{
	if (drawgl_inited)
		Crosshair_LoadImage(s);

	return false;
}

void customCrosshair_Init(void)
{
	FILE *f;
	int i = 0, c;

	customcrosshair_loaded = CROSSHAIR_NONE;

	Crosshair_LoadImage(gl_crosshairimage.string);

	if (FS_FOpenFile("crosshairs/crosshair.txt", &f) == -1)
		return;

	while (i < 64)
	{
		c = fgetc(f);
		if (c == EOF)
		{
			Com_Printf("Invalid format in crosshair.txt (Need 64 X's and O's)\n");	
			fclose(f);
			return;
		}
		if (isspace(c))
			continue;
		if (tolower(c) != 'x' && tolower(c) != 'o')
		{
			Com_Printf("Invalid format in crosshair.txt (Only X's and O's and whitespace permitted)\n");
			fclose(f);
			return;
		}		
		customcrosshairdata[i++] = (c == 'x' || c  == 'X') ? 0xfe : 0xff;
	}
	fclose(f);
	crosshairtexture_txt = GL_LoadTexture ("", 8, 8, customcrosshairdata, TEX_ALPHA, 1);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	customcrosshair_loaded |= CROSSHAIR_TXT;
}

//=============================================================================
/* Support Routines */

#if 0
byte	menuplyr_pixels[4096];
#endif

void Draw_SizeChanged()
{
}

static int Draw_LoadCharset(char *name)
{
	int texnum;

	if (!Q_strcasecmp(name, "original"))
	{
		int i;
		char buf[128 * 256], *src, *dest;

		memset (buf, 255, sizeof(buf));
		src = draw_chars;
		dest = buf;
		for (i = 0; i < 16; i++)
		{
			memcpy (dest, src, 128 * 8);
			src += 128 * 8;
			dest += 128 * 8 * 2;
		}
		char_texture = GL_LoadTexture ("pic:charset", 128, 256, buf, TEX_ALPHA, 1);
	}
	else if ((texnum = GL_LoadCharsetImage (va("textures/charsets/%s", name), "pic:charset")))
	{
		char_texture = texnum;
	}
	else
	{
		Com_Printf ("Couldn't load charset \"%s\"\n", name);
		return 1;
	}

	SetupFontSmoothing();

	return 0;
}

qboolean OnChange_gl_consolefont(cvar_t *var, char *string)
{
	if (drawgl_inited)
		return Draw_LoadCharset(string);

	return 0;
}

void Draw_LoadCharset_f (void)
{
	switch (Cmd_Argc())
	{
		case 1:
			Com_Printf("Current charset is \"%s\"\n", gl_consolefont.string);
			break;
		case 2:
			Cvar_Set(&gl_consolefont, Cmd_Argv(1));
			break;
		default:
			Com_Printf("Usage: %s <charset>\n", Cmd_Argv(0));
			break;
	}
}

void Draw_InitCharset(void)
{
	int i;

	draw_chars = W_GetLumpName ("conchars");
	for (i = 0; i < 256 * 64; i++)
	{
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;
	}

	Draw_LoadCharset(gl_consolefont.string);

	if (!char_texture)
	{
		Cvar_Set(&gl_consolefont, "original");
		Draw_LoadCharset(gl_consolefont.string);
	}

	if (!char_texture)	
		Sys_Error("Draw_InitCharset: Couldn't load charset");
}

void DrawImp_CvarInit(void)
{
	Cmd_AddCommand("loadcharset", Draw_LoadCharset_f);	

	Cvar_SetCurrentGroup(CVAR_GROUP_CONSOLE);
	Cvar_Register (&gl_smoothfont);
	Cvar_Register (&gl_consolefont);	

	Cvar_SetCurrentGroup(CVAR_GROUP_CROSSHAIR);
	Cvar_Register (&gl_crosshairimage);	
	Cvar_Register (&gl_crosshairalpha);

	Cvar_ResetCurrentGroup();

	GL_Texture_CvarInit();
}

void DrawImp_Init(void)
{
	int i;

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	// load the console background and the charset by hand, because we need to write the version
	// string into the background before turning it into a texture
	Draw_InitCharset ();

	// Load the crosshair pics
	for (i = 0; i < NUMCROSSHAIRS; i++)
	{
		crosshairtextures[i] = GL_LoadTexture ("", 8, 8, crosshairdata[i], TEX_ALPHA, 1);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	customCrosshair_Init();		

	drawgl_inited = 1;
}

void DrawImp_Shutdown()
{
	if (crosshairpic)
	{
		Draw_FreePicture(crosshairpic);
		crosshairpic = 0;
	}

	drawgl_inited = 0;
}

__inline static void Draw_CharPoly(int x, int y, int num)
{
	float coords[4*2];
	float texcoords[4*2];
	float frow, fcol;

	frow = (num >> 4) * 0.0625;
	fcol = (num & 15) * 0.0625;

	coords[0*2 + 0] = x;
	coords[0*2 + 1] = y;
	coords[1*2 + 0] = x + 8;
	coords[1*2 + 1] = y;
	coords[2*2 + 0] = x + 8;
	coords[2*2 + 1] = y + 8;
	coords[3*2 + 0] = x;
	coords[3*2 + 1] = y + 8;

	texcoords[0*2 + 0] = fcol;
	texcoords[0*2 + 1] = frow;
	texcoords[1*2 + 0] = fcol + 0.0625;
	texcoords[1*2 + 1] = frow;
	texcoords[2*2 + 0] = fcol + 0.0625;
	texcoords[2*2 + 1] = frow + 0.03125;
	texcoords[3*2 + 0] = fcol;
	texcoords[3*2 + 1] = frow + 0.03125;

	GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, coords);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glDrawArrays(GL_QUADS, 0, 4);
}

static int textrenderingenabled;
static int colouredtextrendering;

#define NUMBUFFEREDTEXTVERTICES 64
#if (NUMBUFFEREDTEXTVERTICES%4) != 0
#error Fail.
#endif

static float fontvertices[2*NUMBUFFEREDTEXTVERTICES] __attribute__((aligned(64)));
static float fonttexcoords[2*NUMBUFFEREDTEXTVERTICES] __attribute__((aligned(64)));
static unsigned int fontcolours[NUMBUFFEREDTEXTVERTICES] __attribute__((aligned(64)));
static int fontindex;

union
{
	unsigned char uc[4];
	unsigned int ui;
} fontcolour =
{
	{ 255, 255, 255, 255 }
};

static void Draw_CharPolyArray(int x, int y, int num)
{
	int index;

	float frow, fcol;

	frow = (num >> 4) * 0.0625;
	fcol = (num & 15) * 0.0625;

	index = fontindex;

	fontvertices[index++] = x;
	fontvertices[index++] = y;

	fontvertices[index++] = x + 8;
	fontvertices[index++] = y;

	fontvertices[index++] = x + 8;
	fontvertices[index++] = y + 8;

	fontvertices[index++] = x;
	fontvertices[index++] = y + 8;

	index = fontindex;

	fonttexcoords[index++] = fcol;
	fonttexcoords[index++] = frow;

	fonttexcoords[index++] = fcol + 0.0625;
	fonttexcoords[index++] = frow;

	fonttexcoords[index++] = fcol + 0.0625;
	fonttexcoords[index++] = frow + 0.03125;

	fonttexcoords[index++] = fcol;
	fonttexcoords[index++] = frow + 0.03125;

	fontindex += 8;

	if (fontindex >= NUMBUFFEREDTEXTVERTICES*2)
	{
		glDrawArrays(GL_QUADS, 0, fontindex/2);
		fontindex = 0;
	}
}

static void Draw_CharPolyColourArray(int x, int y, int num)
{
	int index;
	float frow, fcol;

	frow = (num >> 4) * 0.0625;
	fcol = (num & 15) * 0.0625;

	index = fontindex;

	fontvertices[index++] = x;
	fontvertices[index++] = y;

	fontvertices[index++] = x + 8;
	fontvertices[index++] = y;

	fontvertices[index++] = x + 8;
	fontvertices[index++] = y + 8;

	fontvertices[index++] = x;
	fontvertices[index++] = y + 8;

	index = fontindex;

	fonttexcoords[index++] = fcol;
	fonttexcoords[index++] = frow;

	fonttexcoords[index++] = fcol + 0.0625;
	fonttexcoords[index++] = frow;

	fonttexcoords[index++] = fcol + 0.0625;
	fonttexcoords[index++] = frow + 0.03125;

	fonttexcoords[index++] = fcol;
	fonttexcoords[index++] = frow + 0.03125;

	index = fontindex / 2;

	fontcolours[index+0] = fontcolour.ui;
	fontcolours[index+1] = fontcolour.ui;
	fontcolours[index+2] = fontcolour.ui;
	fontcolours[index+3] = fontcolour.ui;

	fontindex += 8;

	if (fontindex >= NUMBUFFEREDTEXTVERTICES*2)
	{
		glDrawArrays(GL_QUADS, 0, fontindex/2);
		fontindex = 0;
	}
}

void DrawImp_SetTextColor(int r, int g, int b)
{
	fontcolour.uc[0] = r|(r<<4);
	fontcolour.uc[1] = g|(g<<4);
	fontcolour.uc[2] = b|(b<<4);
}

//Draws one 8*8 graphics character with 0 being transparent.
//It can be clipped to the top of the screen to allow the console to be smoothly scrolled off.
void DrawImp_Character(int x, int y, unsigned char num)
{
	if (textrenderingenabled)
	{
		if (colouredtextrendering)
			Draw_CharPolyColourArray(x, y, num);
		else
			Draw_CharPolyArray(x, y, num);

		return;
	}

	GL_Bind(char_texture);

	Draw_CharPoly(x, y, num);
}

void Draw_BeginTextRendering()
{
	if (!textrenderingenabled)
	{
		GL_Bind(char_texture);
		GL_SetAlphaTestBlend(1, 0);
		GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0, fontvertices);
		glTexCoordPointer(2, GL_FLOAT, 0, fonttexcoords);

		fontindex = 0;
	}

	textrenderingenabled++;
}

void Draw_EndTextRendering()
{
	textrenderingenabled--;

	if (!textrenderingenabled)
	{
		if (fontindex)
			glDrawArrays(GL_QUADS, 0, fontindex/2);

		if (colouredtextrendering)
			colouredtextrendering = 0;
	}
}

void Draw_BeginColoredTextRendering()
{
	if (!textrenderingenabled)
	{
		GL_Bind(char_texture);
		GL_SetAlphaTestBlend(1, 0);
		GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_COLOR_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0, fontvertices);
		glTexCoordPointer(2, GL_FLOAT, 0, fonttexcoords);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, fontcolours);

		fontindex = 0;

		colouredtextrendering = 1;
	}

	textrenderingenabled++;
}

void Draw_EndColoredTextRendering()
{
	Draw_EndTextRendering();
	fontcolour.ui = 0xffffffff;
}

void Draw_Crosshair(void)
{
	float x, y, ofs1, ofs2, sh, th, sl, tl;
	byte *col;
	extern vrect_t scr_vrect;
	float coords[4*2];
	static const float texcoords[4*2] =
	{
		0, 0,
		1, 0,
		1, 1,
		0, 1,
	};

	if ((crosshair.value >= 2 && crosshair.value <= NUMCROSSHAIRS + 1)
	 || ((customcrosshair_loaded & CROSSHAIR_TXT) && crosshair.value == 1)
	 || (customcrosshair_loaded & CROSSHAIR_IMAGE))
	{
		x = scr_vrect.x + scr_vrect.width / 2 + cl_crossx.value; 
		y = scr_vrect.y + scr_vrect.height / 2 + cl_crossy.value;

		if (!gl_crosshairalpha.value)
			return;

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		col = StringToRGB(crosshaircolor.string);

		if (gl_crosshairalpha.value)
		{
			GL_SetAlphaTestBlend(0, 1);
			col[3] = bound(0, gl_crosshairalpha.value, 1) * 255;
			glColor4ubv (col);
		}
		else
		{
			GL_SetAlphaTestBlend(0, 0);
			glColor3ubv (col);
		}


		if (customcrosshair_loaded & CROSSHAIR_IMAGE)
		{
			GL_Bind(crosshairpic->texnum);
			ofs1 = 4 - 4.0 / 16;
			ofs2 = 4 + 4.0 / 16;
		}
		else
		{
			GL_Bind ((crosshair.value >= 2) ? crosshairtextures[(int) crosshair.value - 2] : crosshairtexture_txt);	
			ofs1 = 3.5;
			ofs2 = 4.5;
		}
		ofs1 *= (vid.conwidth / 320) * bound(0, crosshairsize.value, 20);
		ofs2 *= (vid.conwidth / 320) * bound(0, crosshairsize.value, 20);

		coords[0*2 + 0] = x - ofs1;
		coords[0*2 + 1] = y - ofs1;
		coords[1*2 + 0] = x + ofs2;
		coords[1*2 + 1] = y - ofs1;
		coords[2*2 + 0] = x + ofs2;
		coords[2*2 + 1] = y + ofs2;
		coords[3*2 + 0] = x - ofs1;
		coords[3*2 + 1] = y + ofs2;

		GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, coords);
		glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
		glDrawArrays(GL_QUADS, 0, 4);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glColor3ubv(color_white);
	}
	else if (crosshair.value)
	{
		Draw_Character(scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx.value, scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy.value, '+');
	}
}

void Draw_AlphaFill(int x, int y, int w, int h, int c, float alpha)
{
	alpha = bound(0, alpha, 1);
	float coords[4*2];

	if (!alpha)
		return;

	glDisable(GL_TEXTURE_2D);
	GL_SetAlphaTestBlend(0, alpha < 1);
	if (alpha < 1)
	{
		glColor4f(host_basepal[c * 3] / 255.0,  host_basepal[c * 3 + 1] / 255.0, host_basepal[c * 3 + 2] / 255.0, alpha);
	}
	else
	{
		glColor3f(host_basepal[c * 3] / 255.0, host_basepal[c * 3 + 1] / 255.0, host_basepal[c * 3 + 2]  /255.0);
	}

	coords[0 + 0] = x;
	coords[0 + 1] = y;

	coords[2 + 0] = x + w;
	coords[2 + 1] = y;

	coords[4 + 0] = x + w;
	coords[4 + 1] = y + h;

	coords[6 + 0] = x;
	coords[6 + 1] = y + h;

	GL_SetArrays(FQ_GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, coords);
	glDrawArrays(GL_QUADS, 0, 4);

	glEnable(GL_TEXTURE_2D);

	glColor3ubv(color_white);
}

void Draw_Fill(int x, int y, int w, int h, int c)
{
	Draw_AlphaFill(x, y, w, h, c, 1);
}

//=============================================================================

void Draw_FadeScreen(void)
{
	float alpha;
	float coords[4*2];

	alpha = bound(0, scr_menualpha.value, 1);
	if (!alpha)
		return;

	if (alpha < 1)
	{
		GL_SetAlphaTestBlend(0, 1);
		glColor4f(0, 0, 0, alpha);
	}
	else
	{
		GL_SetAlphaTestBlend(0, 0);
		glColor3f(0, 0, 0);
	}
	glDisable(GL_TEXTURE_2D);

	coords[0 + 0] = 0;
	coords[0 + 1] = 0;

	coords[2 + 0] = vid.conwidth;
	coords[2 + 1] = 0;

	coords[4 + 0] = vid.conwidth;
	coords[4 + 1] = vid.conheight;

	coords[6 + 0] = 0;
	coords[6 + 1] = vid.conheight;

	GL_SetArrays(FQ_GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, coords);
	glDrawArrays(GL_QUADS, 0, 4);

	glColor3ubv(color_white);
	glEnable(GL_TEXTURE_2D);

	Sbar_Changed();
}

void Draw_SetSize(unsigned int width, unsigned int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -99999, 99999);
}

//=============================================================================

void GL_Set2D(void)
{
	glViewport(glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vid.conwidth, vid.conheight, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glColor3ubv(color_white);
}

struct WadHeader
{
	unsigned int width;
	unsigned short height;
};

struct LmpHeader
{
	unsigned int width;
	unsigned int height;
};

static void *Draw_LoadWadPicture(const char *name, unsigned int *rwidth, unsigned int *rheight)
{
	struct WadHeader *header;
	unsigned int width;
	unsigned int height;
	void *data;
	void *newdata;

	data = W_GetLumpName(name);

	if (data) /* Always true, Quake sucks. */
	{
		header = data;

		width = header->width;
		height = header->height;

		if (width < 32768 && height < 32768)
		{
			/* More memory allocs than needed, oh well... */
			newdata = malloc(width * height);
			if (newdata)
			{
				memcpy(newdata, data + 8, width * height);
				*rwidth = width;
				*rheight = height;
				return newdata;
			}
		}
	}

	return 0;
}

static void *Draw_LoadLmpPicture(FILE *fh, unsigned int *rwidth, unsigned int *rheight)
{
	struct LmpHeader header;
	void *data;
	int r;
	unsigned int width;
	unsigned int height;
	unsigned int size;

	r = fread(&header, 1, sizeof(header), fh);
	if (r == sizeof(header))
	{
		width = LittleLong(header.width);
		height = LittleLong(header.height);

		if (width < 32768 && height < 32768)
		{
			size = width * height;

			data = malloc(size);
			if (data)
			{
				r = fread(data, 1, size, fh);
				if (r == size)
				{
					*rwidth = width;
					*rheight = height;

					return data;
				}

				free(data);
			}
		}
	}

	return 0;
}

static void *Draw_8to32(unsigned char *source, unsigned int width, unsigned int height, GLint *internalformat)
{
	unsigned int *dst;
	int doalpha;
	unsigned int i;

	if (width >= 32768 || height >= 32768)
		return 0;

	dst = malloc(width*height*sizeof(*dst));
	if (dst)
	{
		for(i=0;i<width*height;i++)
		{
			dst[i] = d_8to24table[source[i]];
		}

		*internalformat = 4;
	}

	return dst;
}

#warning Fixme
#define ISPOT(x) (1)

struct Picture *Draw_LoadPicture(const char *name, enum Draw_LoadPicture_Fallback fallback)
{
	char *newname;
	char *newnameextension;
	FILE *fh;
	unsigned int namelen;
	struct Picture *picture;
	GLint internalformat;
	unsigned int width;
	unsigned int height;
	void *data;
	void *newdata;

	data = 0;
	newdata = 0;
	picture = 0;
	internalformat = 4;

	if (strncmp(name, "wad:", 4) == 0)
	{
		namelen = strlen(name + 4);
		newname = malloc(namelen + strlen("textures/wad/") + 1);
		if (newname)
		{
			memcpy(newname, "textures/wad/", strlen("textures/wad/"));
			memcpy(newname + strlen("textures/wad/"), name + 4, namelen);
			newname[strlen("textures/wad/") + namelen] = 0;
			picture = Draw_LoadPicture(newname, DRAW_LOADPICTURE_NOFALLBACK);
			free(newname);
			if (picture)
				return picture;
		}
		newname = malloc(namelen + strlen("gfx/") + 1);
		if (newname)
		{
			memcpy(newname, "gfx/", strlen("gfx/"));
			memcpy(newname + strlen("gfx/"), name + 4, namelen);
			newname[strlen("gfx/") + namelen] = 0;
			picture = Draw_LoadPicture(newname, DRAW_LOADPICTURE_NOFALLBACK);
			free(newname);
			if (picture)
				return picture;
		}
		data = Draw_LoadWadPicture(name + 4, &width, &height);
	}
	else
	{
		namelen = strlen(name);

		newname = malloc(namelen + 4 + 1);
		if (newname)
		{
			COM_StripExtension(name, newname);

			newnameextension = newname + strlen(newname);

			strcpy(newnameextension, ".tga");
			newdata = Image_LoadTGA(0, newname, 0, 0, &width, &height);
#if USE_PNG
			if (!newdata)
			{
				strcpy(newnameextension, ".png");
				newdata = Image_LoadPNG(0, newname, 0, 0, &width, &height);
			}
#endif

			free(newname);
		}

		if (!newdata)
		{
			if (namelen > 4 && strcmp(name + namelen - 4, ".lmp") == 0)
			{
				FS_FOpenFile(name, &fh);
				if (fh)
				{
					data = Draw_LoadLmpPicture(fh, &width, &height);

					fclose(fh);
				}
			}
		}
	}

	if (data)
	{
		newdata = Draw_8to32(data, width, height, &internalformat);

		free(data);
	}

	if (newdata)
	{
		if (ISPOT(width) && ISPOT(height))
		{
			picture = malloc(sizeof(*picture));
			if (picture)
			{
				picture->texnum = texture_extension_number++;
				picture->invwidth = 1.0/width;
				picture->invheight = 1.0/height;

				GL_Bind(picture->texnum);
				glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, newdata);

				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		}

		free(newdata);
	}

	if (picture)
		return picture;

#warning Fixme
	if (fallback == DRAW_LOADPICTURE_DUMMYFALLBACK)
		return 42;

	return 0;
}

void Draw_FreePicture(struct Picture *picture)
{
	glDeleteTextures(1, &picture->texnum);

	free(picture);
}

void Draw_DrawPicture(struct Picture *picture, int x, int y, unsigned int width, unsigned int height)
{
	float coords[4*2];
	static const float texcoords[4*2] =
	{
		0, 0,
		1, 0,
		1, 1,
		0, 1,
	};

	GL_Bind(picture->texnum);

	coords[0*2 + 0] = x;
	coords[0*2 + 1] = y;
	coords[1*2 + 0] = x + width;
	coords[1*2 + 1] = y;
	coords[2*2 + 0] = x + width;
	coords[2*2 + 1] = y + height;
	coords[3*2 + 0] = x;
	coords[3*2 + 1] = y + height;

	GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, coords);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	glDrawArrays(GL_QUADS, 0, 4);
}

void Draw_DrawPictureAlpha(struct Picture *picture, int x, int y, unsigned int width, unsigned int height, float alpha)
{
	float coords[4*2];
	static const float texcoords[4*2] =
	{
		0, 0,
		1, 0,
		1, 1,
		0, 1,
	};

	if (alpha < 0)
		alpha = 0;

	if (alpha > 1)
		alpha = 1;

	GL_SetAlphaTestBlend(0, 1);
	glColor4f(1, 1, 1, alpha);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Bind(picture->texnum);

	coords[0*2 + 0] = x;
	coords[0*2 + 1] = y;
	coords[1*2 + 0] = x + width;
	coords[1*2 + 1] = y;
	coords[2*2 + 0] = x + width;
	coords[2*2 + 1] = y + height;
	coords[3*2 + 0] = x;
	coords[3*2 + 1] = y + height;

	GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, coords);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	glDrawArrays(GL_QUADS, 0, 4);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glColor3ubv(color_white);
}

void Draw_DrawSubPicture(struct Picture *picture, unsigned int sx, unsigned int sy, unsigned int swidth, unsigned int sheight, int x, int y, unsigned int width, unsigned int height)
{
	float coords[4*2];
	float texcoords[4*2];

	GL_Bind(picture->texnum);

	coords[0*2 + 0] = x;
	coords[0*2 + 1] = y;
	coords[1*2 + 0] = x + width;
	coords[1*2 + 1] = y;
	coords[2*2 + 0] = x + width;
	coords[2*2 + 1] = y + height;
	coords[3*2 + 0] = x;
	coords[3*2 + 1] = y + height;

	texcoords[0*2 + 0] = (sx) * picture->invwidth;
	texcoords[0*2 + 1] = (sy) * picture->invheight;
	texcoords[1*2 + 0] = (sx + swidth) * picture->invwidth;
	texcoords[1*2 + 1] = (sy) * picture->invheight;
	texcoords[2*2 + 0] = (sx + swidth) * picture->invwidth;
	texcoords[2*2 + 1] = (sy + sheight) * picture->invheight;
	texcoords[3*2 + 0] = (sx) * picture->invwidth;
	texcoords[3*2 + 1] = (sy + sheight) * picture->invheight;

	GL_SetArrays(FQ_GL_VERTEX_ARRAY | FQ_GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, coords);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	glDrawArrays(GL_QUADS, 0, 4);
}

