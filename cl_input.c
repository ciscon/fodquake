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
// cl.input.c  -- builds an intended movement command to send to the server

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "quakedef.h"
#include "input.h"
#include "pmove.h"		
#include "teamplay.h"	
#include "movie.h"
#include "mouse.h"
#include "ruleset.h"

cvar_t	cl_nodelta = {"cl_nodelta","0"};
cvar_t	cl_c2spps = {"cl_c2spps","0"};
cvar_t	cl_c2sImpulseBackup = {"cl_c2sImpulseBackup","3"};

cvar_t	cl_smartjump = {"cl_smartjump", "0"};


/*
===============================================================================
KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
===============================================================================
*/

kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_use, in_jump, in_attack;
kbutton_t	in_up, in_down;

int			in_impulse;

static int checkmovementruleset()
{
	/* Ugliest hack in the world */
	extern int movementkey;

	if (Ruleset_AllowMovementScripts())
		return 1;

	if (atoi(Cmd_Argv(2)) != movementkey)
	{
		Com_Printf("Movement scripts disallowed by ruleset\n");
		return 0;
	}

	return 1;
}

static void KeyDown(kbutton_t *b)
{
	int k;
	char *c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Com_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if (b->state & 1)
		return;		// still down

	b->state |= 1 + 2;	// down + impulse down
}

static void KeyUp(kbutton_t *b)
{
	int k;
	char *c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state &= ~1;
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)

	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
}

static void IN_UpDown(void) {KeyDown(&in_up);}
static void IN_UpUp(void) {KeyUp(&in_up);}
static void IN_DownDown(void) {KeyDown(&in_down);}
static void IN_DownUp(void) {KeyUp(&in_down);}
static void IN_LeftDown(void) {KeyDown(&in_left);}
static void IN_LeftUp(void) {KeyUp(&in_left);}
static void IN_RightDown(void) {KeyDown(&in_right);}
static void IN_RightUp(void) {KeyUp(&in_right);}
static void IN_ForwardDown(void) {if (checkmovementruleset()) KeyDown(&in_forward);}
static void IN_ForwardUp(void) {if (checkmovementruleset()) KeyUp(&in_forward);}
static void IN_BackDown(void) {if (checkmovementruleset()) KeyDown(&in_back);}
static void IN_BackUp(void) {if (checkmovementruleset()) KeyUp(&in_back);}
static void IN_LookupDown(void) {KeyDown(&in_lookup);}
static void IN_LookupUp(void) {KeyUp(&in_lookup);}
static void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
static void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
static void IN_MoveleftDown(void) {if (checkmovementruleset()) KeyDown(&in_moveleft);}
static void IN_MoveleftUp(void) {if (checkmovementruleset()) KeyUp(&in_moveleft);}
static void IN_MoverightDown(void) {if (checkmovementruleset()) KeyDown(&in_moveright);}
static void IN_MoverightUp(void) {if (checkmovementruleset()) KeyUp(&in_moveright);}

static void IN_AttackDown(void) {KeyDown(&in_attack);}
static void IN_AttackUp(void) {KeyUp(&in_attack);}

static void IN_UseDown (void) {KeyDown(&in_use);}
static void IN_UseUp (void) {KeyUp(&in_use);}


static void IN_JumpDown(void) {
	qboolean condition;

	condition = (cls.state == ca_active && cl_smartjump.value);
	if (condition && cl.stats[STAT_HEALTH] > 0 && !cls.demoplayback && !cl.spectator && 
		cl.waterlevel >= 2 && (!cl.teamfortress || !(in_forward.state & 1))
	)
		KeyDown(&in_up);
	else if (condition && cl.spectator && Cam_TrackNum() == -1)
		KeyDown(&in_up);
	else
		KeyDown(&in_jump);
}

static void IN_JumpUp(void) {
	if (cl_smartjump.value)
		KeyUp(&in_up);
	KeyUp(&in_jump);
}



//Tonik void IN_Impulse (void) {in_impulse=Q_atoi(Cmd_Argv(1));}

// Tonik -->
static void IN_Impulse (void) {
	int best, i, imp, items;

	in_impulse = Q_atoi(Cmd_Argv(1));

	if (Cmd_Argc() <= 2)
		return;

	items = cl.stats[STAT_ITEMS];
	best = 0;

	for (i = Cmd_Argc() - 1; i > 0; i--) {
		imp = Q_atoi(Cmd_Argv(i));
		if (imp < 1 || imp > 8)
			continue;

		switch (imp) {
			case 1:
				if (items & IT_AXE)
					best = 1;
				break;
			case 2:
				if (items & IT_SHOTGUN && cl.stats[STAT_SHELLS] >= 1)
					best = 2;
				break;
			case 3:
				if (items & IT_SUPER_SHOTGUN && cl.stats[STAT_SHELLS] >= 2)
					best = 3;
				break;
			case 4:
				if (items & IT_NAILGUN && cl.stats[STAT_NAILS] >= 1)
					best = 4;
				break;
			case 5:
				if (items & IT_SUPER_NAILGUN && cl.stats[STAT_NAILS] >= 2)
					best = 5;
				break;
			case 6:
				if (items & IT_GRENADE_LAUNCHER && cl.stats[STAT_ROCKETS] >= 1)
					best = 6;
				break;
			case 7:
				if (items & IT_ROCKET_LAUNCHER && cl.stats[STAT_ROCKETS] >= 1)
					best = 7;
				break;
			case 8:
				if (items & IT_LIGHTNING && cl.stats[STAT_CELLS] >= 1)
					best = 8;
		}
	}
	
	if (best)
		in_impulse = best;
}
// <-- Tonik

/*
Does not return 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, or
1.0 if held for the entire time

 - because that's just retarded.

Returns 1 if a key is pressed, 0 otherwise.
*/
static float CL_KeyState (kbutton_t *key) {
	float val;
	
	val = !!key->state;

	key->state &= 1;		// clear impulses
	
	return val;
}

//==========================================================================

static cvar_t	cl_upspeed = {"cl_upspeed","400"};
static cvar_t	cl_forwardspeed = {"cl_forwardspeed","400",CVAR_ARCHIVE};
static cvar_t	cl_backspeed = {"cl_backspeed","400",CVAR_ARCHIVE};
static cvar_t	cl_sidespeed = {"cl_sidespeed","400",CVAR_ARCHIVE};

static cvar_t	cl_yawspeed = {"cl_yawspeed","140"};
static cvar_t	cl_pitchspeed = {"cl_pitchspeed","150"};

static void CL_Rotate_f (void)
{
	vec3_t angles;

	if (Cmd_Argc() != 2) {
		Com_Printf("Usage: %s <degrees>\n", Cmd_Argv(0));
		return;
	}
	if ((cl.fpd & FPD_LIMIT_YAW) || !Ruleset_AllowRJScripts())
		return;

	angles[PITCH] = 0;
	angles[ROLL] = 0;
	angles[YAW] = atof(Cmd_Argv(1));

	Mouse_AddViewAngles(angles);
}

static void CL_Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

//Send the intended movement message to the server
void CL_BaseMove (usercmd_t *cmd) {	
	memset (cmd, 0, sizeof(*cmd));
	
	VectorCopy (cl.viewangles, cmd->angles);

	cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
	cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed.value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed.value * CL_KeyState (&in_down);

	cmd->forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
	cmd->forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
}

static short MakeShort (int i)
{
	if (i < -32768)
		i = -32768;
	if (i > 32767)
		i = 32767;

	return i;
}

void CL_FinishMove (usercmd_t *cmd) {
	int i, ms;
	float frametime;
	static double extramsec = 0;

	if (Movie_IsCapturing() && movie_steadycam.value)
		frametime = movie_fps.value > 0 ? 1.0 / movie_fps.value : 1 / 30.0;
	else
		frametime = cls.trueframetime;

	// figure button bits
	if ( in_attack.state & 3 )
		cmd->buttons |= 1;
	in_attack.state &= ~2;

	if (in_jump.state & 3)
		cmd->buttons |= 2;
	in_jump.state &= ~2;

	if (in_use.state & 3)
		cmd->buttons |= 4;
	in_use.state &= ~2;

	// send milliseconds of time to apply the move
	extramsec += frametime * 1000;
	ms = extramsec;
	extramsec -= ms;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	VectorCopy (cl.viewangles, cmd->angles);

	cmd->impulse = in_impulse;
	in_impulse = 0;

	// chop down so no extra bits are kept that the server wouldn't get
	cmd->forwardmove = MakeShort(cmd->forwardmove);
	cmd->sidemove = MakeShort(cmd->sidemove);
	cmd->upmove = MakeShort(cmd->upmove);

	for (i = 0; i < 3; i++)
		cmd->angles[i] = (Q_rint(cmd->angles[i] * 65536.0 / 360.0) & 65535) * (360.0 / 65536.0);
}

#ifndef NETQW
void CL_SendCmd (void) {
	sizebuf_t buf;
	byte data[256];
	usercmd_t *cmd, *oldcmd;
	int i, checksumIndex, lost;
	qboolean dontdrop;
	static float pps_balance = 0;
	static int dropcount = 0;

	if (cls.demoplayback && !cls.mvdplayback)	
		return; // sendcmds come from the demo

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & UPDATE_MASK;
	cmd = &cl.frames[i].cmd;
	cl.frames[i].senttime = cls.realtime;
	cl.frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	// get basic movement from keyboard
	CL_BaseMove (cmd);

	Mouse_GetViewAngles(cl.viewangles);

	// if we are spectator, try autocam
	if (cl.spectator)
		Cam_Track(cmd);

	CL_FinishMove(cmd);

	Cam_FinishMove(cmd);

	if (cls.mvdplayback) {		
		cls.netchan.outgoing_sequence++;
		return;
	}

	SZ_Init (&buf, data, sizeof(data)); 

	// begin a client move command
	MSG_WriteByte (&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte (&buf, 0);

	// write our lossage percentage
	lost = CL_CalcNet();
	MSG_WriteByte (&buf, (byte)lost);

	// send this and the previous two cmds in the message, so if the last packet was dropped, it can be recovered
	dontdrop = false;

	i = (cls.netchan.outgoing_sequence - 2) & UPDATE_MASK;
	cmd = &cl.frames[i].cmd;
	if (cl_c2sImpulseBackup.value >= 2)
		dontdrop = dontdrop || cmd->impulse;
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence - 1) & UPDATE_MASK;
	cmd = &cl.frames[i].cmd;
	if (cl_c2sImpulseBackup.value >= 3)
		dontdrop = dontdrop || cmd->impulse;
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & UPDATE_MASK;
	cmd = &cl.frames[i].cmd;
	if (cl_c2sImpulseBackup.value >= 1)
		dontdrop = dontdrop || cmd->impulse;
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	// request delta compression of entities
	if (cls.netchan.outgoing_sequence - cl.validsequence >= UPDATE_BACKUP - 1) {
		cl.validsequence = 0;
		cl.delta_sequence = 0;
	}

	if (cl.delta_sequence && !cl_nodelta.value && cls.state == ca_active) {
		cl.frames[cls.netchan.outgoing_sequence & UPDATE_MASK].delta_sequence = cl.delta_sequence;
		MSG_WriteByte (&buf, clc_delta);
		MSG_WriteByte (&buf, cl.delta_sequence & 255);
	} else {
		cl.frames[cls.netchan.outgoing_sequence & UPDATE_MASK].delta_sequence = -1;
	}

	if (cls.demorecording)
		CL_WriteDemoCmd (cmd);

	if (cl_c2spps.value) {
		pps_balance += cls.frametime;
		// never drop more than 2 messages in a row -- that'll cause PL
		// and don't drop if one of the last two movemessages have an impulse
		if (pps_balance > 0 || dropcount >= 2 || dontdrop) {
			float	pps;
			pps = cl_c2spps.value;
			if (pps < 10) pps = 10;
			if (pps > 72) pps = 72;
			pps_balance -= 1 / pps;
			// bound pps_balance. FIXME: is there a better way?
			if (pps_balance > 0.1) pps_balance = 0.1;
			if (pps_balance < -0.1) pps_balance = -0.1;
			dropcount = 0;
		} else {
			// don't count this message when calculating PL
			cl.frames[i].receivedtime = -3;
			// drop this message
			cls.netchan.outgoing_sequence++;
			dropcount++;
			return;
		}
	} else {
		pps_balance = 0;
		dropcount = 0;
	}

	if (cls.download && (cls.ftexsupported&FTEX_CHUNKEDDOWNLOADS))
	{
		CL_RequestNextFTEDownloadChunk(&buf);
	}

	// deliver the message
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);	
}
#endif

void CL_CvarInitInput(void)
{
	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+attack", IN_AttackDown);
	Cmd_AddCommand ("-attack", IN_AttackUp);
	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("+jump", IN_JumpDown);
	Cmd_AddCommand ("-jump", IN_JumpUp);
	Cmd_AddCommand ("impulse", IN_Impulse);

	
	Cmd_AddCommand ("rotate",CL_Rotate_f);

	Cmd_AddCommand ("force_centerview", CL_Force_CenterView_f);

	Cvar_SetCurrentGroup(CVAR_GROUP_INPUT_KEYBOARD);

	Cvar_Register (&cl_smartjump);


	Cvar_Register (&cl_upspeed);
	Cvar_Register (&cl_forwardspeed);
	Cvar_Register (&cl_backspeed);
	Cvar_Register (&cl_sidespeed);
	Cvar_Register (&cl_yawspeed);
	Cvar_Register (&cl_pitchspeed);

	Cvar_SetCurrentGroup(CVAR_GROUP_NETWORK);
	Cvar_Register (&cl_nodelta);
	Cvar_Register (&cl_c2sImpulseBackup);
	Cvar_Register (&cl_c2spps);

	Cvar_ResetCurrentGroup();
}

