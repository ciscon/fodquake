/*
Copyright (C) 2010 Mark Olsen

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

#include <dinput.h>

#include "quakedef.h"
#include "input.h"
#include "keys.h"
#include "winquake2.h"
#include "in_dinput8.h"

#define DINPUTNUMEVENTS 32
#define NUMBUTTONEVENTS 32

struct buttonevent
{
	unsigned char key;
	unsigned char down;
};

struct InputData
{
	LPDIRECTINPUT8 di8;
	LPDIRECTINPUTDEVICE8 di8mouse;
	LPDIRECTINPUTDEVICE8 di8keyboard;

	int mousex;
	int mousey;

	struct buttonevent buttonevents[NUMBUTTONEVENTS];
	unsigned int buttoneventhead;
	unsigned int buttoneventtail;

	unsigned char shiftdown;
};

#warning Complete this.
static const unsigned char shiftkeytable[] =
{
	0, /* 0 */
	0,
	'!',
	'@',
	'#',
	'$',
	'%',
	'^',
	'&',
	'*',
	'(', /* 10 */
	')',
	'_',
	'+',
	0,
	0,
	'Q',
	'W',
	'E',
	'R',
	'T', /* 20 */
	'Y',
	'U',
	'I',
	'O',
	'P',
	'{',
	'}',
	0,
	0,
	'A', /* 30 */
	'S',
	'D',
	'F',
	'G',
	'H',
	'J',
	'K',
	'L',
	':',
	'"', /* 40 */
	'~',
	0,
	'|',
	'Z',
	'X',
	'C',
	'V',
	'B',
	'N',
	'M', /* 50 */
	'<',
	'>',
	'?',
};

static const unsigned char keytable[] =
{
	0, /* 0 */
	K_ESCAPE,
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9', /* 10 */
	'0',
	'-',
	'=',
	K_BACKSPACE,
	K_TAB,
	'q',
	'w',
	'e',
	'r',
	't', /* 20 */
	'y',
	'u',
	'i',
	'o',
	'p',
	'[',
	']',
	K_ENTER,
	K_LCTRL,
	'a', /* 30 */
	's',
	'd',
	'f',
	'g',
	'h',
	'j',
	'k',
	'l',
	';',
	'\'', /* 40 */
	'`',
	K_LSHIFT,
	'\\',
	'z',
	'x',
	'c',
	'v',
	'b',
	'n',
	'm', /* 50 */
	',',
	'.',
	'/',
	K_RSHIFT,
	KP_STAR,
	0,
	' ',
	0,
	K_F1,
	K_F2, /* 60 */
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	0,
	0, /* 70 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 80 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 90 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 100 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 110 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 120 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 130 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 140 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 150 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 160 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 170 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 180 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 190 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	K_UPARROW, /* 200 */
	0,
	0,
	K_LEFTARROW,
	0,
	K_RIGHTARROW,
	0,
	0,
	K_DOWNARROW,
	0,
	0, /* 210 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

struct InputData *Sys_Input_Init(HWND window)
{
	struct InputData *id;
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = DINPUTNUMEVENTS;

	id = malloc(sizeof(*id));
	if (id)
	{
		if (DirectInput8Create(global_hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&id->di8, 0) == DI_OK)
		{
			if (id->di8->lpVtbl->CreateDevice(id->di8, &GUID_SysMouse, &id->di8mouse, 0) == DI_OK)
			{
				if (id->di8mouse->lpVtbl->SetDataFormat(id->di8mouse, &c_dfDIMouse2) == DI_OK)
				{
					if (id->di8mouse->lpVtbl->SetCooperativeLevel(id->di8mouse, window, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE) == DI_OK)
					{
						if (id->di8mouse->lpVtbl->SetProperty(id->di8mouse, DIPROP_BUFFERSIZE, &dipdw.diph) == DI_OK)
						{
							if (id->di8->lpVtbl->CreateDevice(id->di8, &GUID_SysKeyboard, &id->di8keyboard, 0) == DI_OK)
							{
								if (id->di8keyboard->lpVtbl->SetDataFormat(id->di8keyboard, &c_dfDIKeyboard) == DI_OK)
								{
									if (id->di8keyboard->lpVtbl->SetCooperativeLevel(id->di8keyboard, window, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE|DISCL_NOWINKEY) == DI_OK)
									{
										if (id->di8keyboard->lpVtbl->SetProperty(id->di8keyboard, DIPROP_BUFFERSIZE, &dipdw.diph) == DI_OK)
										{
											id->di8keyboard->lpVtbl->Acquire(id->di8keyboard);

											id->di8mouse->lpVtbl->Acquire(id->di8mouse);

											id->mousex = 0;
											id->mousey = 0;

											id->buttoneventhead = 0;
											id->buttoneventtail = 0;

											id->shiftdown = 0;

											return id;
										}
									}
								}

								id->di8keyboard->lpVtbl->Release(id->di8keyboard);
							}
						}
					}
				}

				id->di8mouse->lpVtbl->Release(id->di8mouse);
			}

			id->di8->lpVtbl->Release(id->di8);
		}

		free(id);
	}

	return 0;
}

void Sys_Input_Shutdown(struct InputData *inputdata)
{
	inputdata->di8keyboard->lpVtbl->Unacquire(inputdata->di8keyboard);
	inputdata->di8keyboard->lpVtbl->Release(inputdata->di8keyboard);

	inputdata->di8mouse->lpVtbl->Unacquire(inputdata->di8mouse);
	inputdata->di8mouse->lpVtbl->Release(inputdata->di8mouse);

	inputdata->di8->lpVtbl->Release(inputdata->di8);

	free(inputdata);
}

static void pollstuff(struct InputData *inputdata)
{
	DIDEVICEOBJECTDATA events[DINPUTNUMEVENTS];
	DWORD elements;
	HRESULT res;
	unsigned int i;

	elements = DINPUTNUMEVENTS;
	res = inputdata->di8mouse->lpVtbl->GetDeviceData(inputdata->di8mouse, sizeof(*events), events, &elements, 0);
	if (res != DI_OK)
	{
#warning Should release all pressed buttons here.

		inputdata->di8mouse->lpVtbl->Acquire(inputdata->di8mouse);
	}
	else
	{
		for(i=0;i<elements;i++)
		{
			switch(events[i].dwOfs)
			{
				case DIMOFS_X:
					inputdata->mousex += events[i].dwData;
					break;
				case DIMOFS_Y:
					inputdata->mousey += events[i].dwData;
					break;
			}
		}
	}

	elements = DINPUTNUMEVENTS;
	res = inputdata->di8keyboard->lpVtbl->GetDeviceData(inputdata->di8keyboard, sizeof(*events), events, &elements, 0);
	if (res != DI_OK)
	{
#warning Should release all pressed buttons here.

		inputdata->di8keyboard->lpVtbl->Acquire(inputdata->di8keyboard);
	}
	else
	{
		for(i=0;i<elements;i++)
		{
			printf("Key %d\n", events[i].dwOfs);

			if (events[i].dwOfs < sizeof(keytable) && keytable[events[i].dwOfs])
			{
				if (keytable[events[i].dwOfs] == K_LSHIFT)
				{
					if ((events[i].dwData&0x80))
						inputdata->shiftdown |= 1;
					else
						inputdata->shiftdown &= ~1;
				}
				else if (keytable[events[i].dwOfs] == K_RSHIFT)
				{
					if ((events[i].dwData&0x80))
						inputdata->shiftdown |= 2;
					else
						inputdata->shiftdown &= ~2;
				}
				if (inputdata->shiftdown && events[i].dwOfs < sizeof(shiftkeytable) && shiftkeytable[events[i].dwOfs])
					inputdata->buttonevents[inputdata->buttoneventhead].key = shiftkeytable[events[i].dwOfs];
				else
					inputdata->buttonevents[inputdata->buttoneventhead].key = keytable[events[i].dwOfs];
				inputdata->buttonevents[inputdata->buttoneventhead].down = !!(events[i].dwData&0x80);
				inputdata->buttoneventhead = (inputdata->buttoneventhead + 1) % NUMBUTTONEVENTS;
			}
		}
	}
}

int Sys_Input_GetKeyEvent(struct InputData *inputdata, keynum_t *keynum, qboolean *down)
{
	if (inputdata->buttoneventhead != inputdata->buttoneventtail)
	{
		*keynum = inputdata->buttonevents[inputdata->buttoneventtail].key;
		*down = inputdata->buttonevents[inputdata->buttoneventtail].down;

		inputdata->buttoneventtail = (inputdata->buttoneventtail + 1) % NUMBUTTONEVENTS;

		return 1;
	}

	return 0;
}

void Sys_Input_GetMouseMovement(struct InputData *inputdata, int *mousex, int *mousey)
{
	pollstuff(inputdata);

	if (inputdata->mousex || inputdata->mousey)
		printf("%d %d\n", inputdata->mousex, inputdata->mousey);

	*mousex = inputdata->mousex;
	*mousey = inputdata->mousey;

	inputdata->mousex = 0;
	inputdata->mousey = 0;
}

void Sys_Input_GrabMouse(struct InputData *inputdata, int dograb)
{
}
