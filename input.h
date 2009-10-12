/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// input.h -- external (non-keyboard) input devices

//
// cl_input.c
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

void CL_CvarInitInput (void);
void CL_SendCmd (void);
void CL_BaseMove (usercmd_t *cmd);
float CL_KeyState (kbutton_t *key);


extern cvar_t	cl_upspeed;
extern cvar_t	cl_forwardspeed;
extern cvar_t	cl_backspeed;
extern cvar_t	cl_sidespeed;
extern cvar_t	cl_yawspeed;
extern cvar_t	cl_pitchspeed;

extern cvar_t	sensitivity;

extern cvar_t	m_pitch;
extern cvar_t	m_yaw;
extern cvar_t	m_forward;
extern cvar_t	m_side;
extern cvar_t	m_accel;

extern cvar_t	in_grab_windowed_mouse;

