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

#include <string.h>
#include <stdlib.h>

#include "quakedef.h"
#include "sys_net.h"
#include "strl.h"

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif

struct netaddr	net_local_adr;

struct netaddr	net_from;
sizebuf_t	net_message;

#define	MAX_UDP_PACKET	(MAX_MSGLEN*2)	// one more than msg + header
byte		net_message_buffer[MAX_UDP_PACKET];

#define	MAX_LOOPBACK	4	// must be a power of two

struct loopmsg
{
	byte data[MAX_UDP_PACKET];
	int datalen;
};

struct loopback
{
	struct loopmsg msgs[MAX_LOOPBACK];
	unsigned int get, send;
};

struct NetData
{
	struct SysNetData *sysnetdata;
	struct loopback loopbacks[2];
	struct SysSocket *sockets[2];
};

struct NetData *netdata;

//=============================================================================

qboolean NET_CompareBaseAdr(struct netaddr a, struct netaddr b)
{
#warning Fix this.
	if (a.type == NA_LOOPBACK && b.type == NA_LOOPBACK)
		return true;
	if (a.addr.ipv4.address[0] == b.addr.ipv4.address[0] && a.addr.ipv4.address[1] == b.addr.ipv4.address[1] && a.addr.ipv4.address[2] == b.addr.ipv4.address[2] && a.addr.ipv4.address[3] == b.addr.ipv4.address[3])
		return true;
	return false;
}

qboolean NET_CompareAdr(struct netaddr a, struct netaddr b)
{
#warning Fix this.
	if (a.type == NA_LOOPBACK && b.type == NA_LOOPBACK)
		return true;
	if (a.addr.ipv4.address[0] == b.addr.ipv4.address[0] && a.addr.ipv4.address[1] == b.addr.ipv4.address[1] && a.addr.ipv4.address[2] == b.addr.ipv4.address[2] && a.addr.ipv4.address[3] == b.addr.ipv4.address[3] && a.addr.ipv4.port == b.addr.ipv4.port)
		return true;
	return false;
}

qboolean NET_IsLocalAddress(const struct netaddr *a)
{
#warning Fix this... Broken in so many ways :)
	if (a->type != NA_IPV4)
		return false;

	if ((*(unsigned *)a->addr.ipv4.address == BigLong(INADDR_LOOPBACK)))
		return true;
	
	return false;
}

static void NET_IPv6ToString(char *s, unsigned int size, const struct netaddr *a)
{
	unsigned int biggestzeroblockstart;
	unsigned int biggestzeroblocksize;
	unsigned int i;
	unsigned int j;
	unsigned int r;

	biggestzeroblockstart = ~0;
	biggestzeroblocksize = 0;

	for(i=0;i<sizeof(a->addr.ipv6.address);i+=2)
	{
		if (a->addr.ipv6.address[i] == 0 && a->addr.ipv6.address[i+1] == 0)
		{
			for(j=2;j+i<sizeof(a->addr.ipv6.address);j+=2)
			{
				if (a->addr.ipv6.address[i+j] != 0 || a->addr.ipv6.address[i+j+1] != 0)
					break;
			}

			if (j > biggestzeroblocksize)
			{
				biggestzeroblocksize = j;
				biggestzeroblockstart = i;
			}
		}
	}

	if (size < 2)
		return;

	s[0] = '[';
	s[1] = 0;

	s++;
	size--;

	for(i=0;i<sizeof(a->addr.ipv6.address);i+=2)
	{
		if (i == biggestzeroblockstart)
		{
			r = snprintf(s, size, ":");
			i+= biggestzeroblocksize - 2;
		}
		else
			r = snprintf(s, size, "%s%x", i!=0?":":"", (a->addr.ipv6.address[i]<<8)|a->addr.ipv6.address[i+1]);

		s+= r;
		size-= r;
	}

	if (size < 2)
		return;

	s[0] = ']';
	s[1] = 0;

	s++;
	size--;

	snprintf(s, size, ":%d", a->addr.ipv6.port);
}

char *NET_AdrToString(struct netaddr a)
{
#warning Fix this.
	static char s[64];

	if (a.type == NA_LOOPBACK)
		return "loopback";
	else if (a.type == NA_IPV4)
		snprintf(s, sizeof(s), "%d.%d.%d.%d:%d", a.addr.ipv4.address[0], a.addr.ipv4.address[1], a.addr.ipv4.address[2], a.addr.ipv4.address[3], a.addr.ipv4.port);
	else if (a.type == NA_IPV6)
		NET_IPv6ToString(s, sizeof(s), &a);
	else
		s[0] = 0;

	return s;
}

char *NET_BaseAdrToString(struct netaddr a)
{
#warning Fix this.
	static char s[64];
	
	snprintf(s, sizeof(s), "%d.%d.%d.%d", a.addr.ipv4.address[0], a.addr.ipv4.address[1], a.addr.ipv4.address[2], a.addr.ipv4.address[3]);

	return s;
}

static unsigned int NET_StringDecToUInt(const char *s, unsigned int *value, unsigned int maxvalue)
{
	unsigned int len;

	len = 0;
	*value = 0;
	while(*s >= '0' && *s <= '9' && *value <= maxvalue)
	{
		*value*= 10;
		*value+= *s-'0';
		len++;
		s++;
	}

	if (*value > maxvalue)
		return 0;

	return len;
}

static unsigned int NET_StringToIPv6(const char *s, struct netaddr *a)
{
	return 0;
}

static unsigned int NET_StringToIPv4(const char *s, struct netaddr *a)
{
	const char *origs;
	unsigned int value;
	unsigned int r;
	unsigned int i;

	origs = s;

	for(i=0;i<4;i++)
	{
		r = NET_StringDecToUInt(s, &value, 255);
		if (r == 0)
			return 0;
		s+= r;
		if ((i == 3 && *s != 0 && *s != ':')
		 || (i != 3 &&*s != '.'))
			return 0;
		s++;
		a->addr.ipv4.address[i] = value;
	}

	a->type = NA_IPV4;

	return s-origs;
}

unsigned int NET_StringToNetAddrNoPort(const char *s, struct netaddr *a)
{
	unsigned int len;

	if (strcmp(s, "local") == 0)
	{
		a->type = NA_LOOPBACK;
		return 5;
	}

	len = NET_StringToIPv6(s, a);
	if (len)
		return len;

	len = NET_StringToIPv4(s, a);
	if (len)
		return len;
	
	{
		char *newstr;
		const char *endofhostname;

		endofhostname = strchr(s, ':');
		if (endofhostname == 0)
			endofhostname = s + strlen(s);

		newstr = malloc(endofhostname - s + 1);
		if (newstr)
		{
			strlcpy(newstr, s, endofhostname - s + 1);

			if (!Sys_Net_ResolveName(netdata->sysnetdata, newstr, a))
			{
				endofhostname = s;
			}

			free(newstr);
		}
		else
			endofhostname = s;

		return endofhostname - s;
	}
}

qboolean NET_StringToAdr(const char *s, struct netaddr *a)
{
	unsigned int len;
	unsigned int port;

	memset(a, 0, sizeof(*a));

	len = NET_StringToNetAddrNoPort(s, a);
	if (len == 0)
		return false;

	s+= len;

	if (*s == ':')
	{
		s++;

		len = NET_StringDecToUInt(s, &port, 65535);
		if (len == 0)
			return false;

		s+= len;
	}
	else
		port = 27500;

	if (*s != 0)
		return false;

	if (a->type == NA_IPV4)
		a->addr.ipv4.port = port;
	else if (a->type == NA_IPV6)
		a->addr.ipv6.port = port;
	else if (port != 27500)
		return false;

	return true;
}


/*
=============================================================================
LOOPBACK BUFFERS FOR LOCAL PLAYER
=============================================================================
*/

static qboolean NET_GetLoopPacket (enum netsrc sock)
{
	int i;
	struct loopback *loop;

	loop = &netdata->loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if ((int)(loop->send - loop->get) <= 0)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message.data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message.cursize = loop->msgs[i].datalen;
	memset (&net_from, 0, sizeof(net_from));
	net_from.type = NA_LOOPBACK;

	return true;
}

static void NET_SendLoopPacket(enum netsrc sock, int length, void *data, const struct netaddr *to)
{
	int i;
	struct loopback *loop;

	loop = &netdata->loopbacks[sock ^ 1];

	i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	if (length > sizeof(loop->msgs[i].data))
		Sys_Error ("NET_SendLoopPacket: length > MAX_UDP_PACKET");

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

void NET_ClearLoopback(void)
{
	if (netdata == 0)
		Sys_Error("NET_ClearLoopback() called before net init\n");

	netdata->loopbacks[0].send = netdata->loopbacks[0].get = 0;
	netdata->loopbacks[1].send = netdata->loopbacks[1].get = 0;
}

qboolean NET_GetPacket(enum netsrc sock)
{
	int ret;
	struct SysSocket *net_socket;

	if (NET_GetLoopPacket (sock))
		return true;

	net_socket = netdata->sockets[sock];
	if (net_socket == 0)
		return false;

	ret = Sys_Net_Receive(netdata->sysnetdata, net_socket, net_message_buffer, sizeof(net_message_buffer), &net_from);

	if (ret <= 0)
	{
		if (ret < 0)
			Com_Printf("%s: Network error.\n", __func__);

		return false;
	}

	net_message.cursize = ret;

	return true;
}

//=============================================================================

void NET_SendPacket(enum netsrc sock, int length, void *data, const struct netaddr *to)
{
	int ret;
	struct SysSocket *net_socket;

	if (to->type == NA_LOOPBACK)
	{
		NET_SendLoopPacket (sock, length, data, to);
		return;
	}

	net_socket = netdata->sockets[sock];
	if (net_socket == 0)
		return;

	ret = Sys_Net_Send(netdata->sysnetdata, net_socket, data, length, to);
	if (ret < 0)
	{
		Com_Printf("%s: Network error.\n", __func__);
	}
}

//=============================================================================

qboolean NET_OpenSocket(enum netsrc socknum, enum netaddrtype type)
{
	if (netdata->sockets[socknum])
		Sys_Net_DeleteSocket(netdata->sysnetdata, netdata->sockets[socknum]);

	netdata->sockets[socknum] = Sys_Net_CreateSocket(netdata->sysnetdata, type);

	if (netdata->sockets[socknum])
		return true;

	return false;
}

#warning Obsolete function.
void NET_ClientConfig(qboolean enable)
{
	if (enable)
	{
		if (netdata->sockets[NS_CLIENT] == 0)
		{
			netdata->sockets[NS_CLIENT] = Sys_Net_CreateSocket(netdata->sysnetdata, NA_IPV4);

			if (netdata->sockets[NS_CLIENT] == 0)
				Com_Printf("WARNING: Couldn't allocate client socket.\n");
		}
	}
	else
	{
		if (netdata->sockets[NS_CLIENT])
		{
			Sys_Net_DeleteSocket(netdata->sysnetdata, netdata->sockets[NS_CLIENT]);
			netdata->sockets[NS_CLIENT] = 0;
		}
	}
}

void NET_ServerConfig (qboolean enable)
{
	int i, port;

	if (enable)
	{
		if (netdata->sockets[NS_SERVER] == 0)
		{
#warning Command line arguments are baaaaaad.
			port = 0;
			i = COM_CheckParm ("-port");
			if (i && i < com_argc)
				port = atoi(com_argv[i+1]);
			if (!port)
				port = PORT_SERVER;

			netdata->sockets[NS_SERVER] = Sys_Net_CreateSocket(netdata->sysnetdata, NA_IPV4);
			if (netdata->sockets[NS_SERVER] == 0)
			{
#ifdef SERVERONLY
				if (1)
#else
				if (dedicated)
#endif
					Sys_Error ("Couldn't allocate server socket");
				else
					Com_Printf ("WARNING: Couldn't allocate server socket.\n");
			}
			else
			{
				if (!Sys_Net_Bind(netdata->sysnetdata, netdata->sockets[NS_SERVER], port))
				{
#ifdef SERVERONLY
					if (1)
#else
					if (dedicated)
#endif
						Sys_Error("Unable to bind server socket to port");
					else
						Com_Printf("WARNING: Unable to bind server socket to port.\n");
				}
				else
				{
					return;
				}

				Sys_Net_DeleteSocket(netdata->sysnetdata, netdata->sockets[NS_SERVER]);
				netdata->sockets[NS_SERVER] = 0;
			}
		}
	}
	else
	{
		if (netdata->sockets[NS_SERVER] != 0)
		{
			Sys_Net_DeleteSocket(netdata->sysnetdata, netdata->sockets[NS_SERVER]);
			netdata->sockets[NS_SERVER] = 0;
		}
	}
}

//Sleeps msec or until the server socket is ready
void NET_Sleep (int msec)
{
#warning WTF function.

#if 0
	struct timeval timeout;
	fd_set fdset;
	extern qboolean do_stdin, stdin_ready;

	if (dedicated)
	{
		if (netdata->ip_sockets[NS_SERVER] == -1)
			return; // we're not a server, just run full speed

		FD_ZERO (&fdset);
		if (do_stdin)
			FD_SET (0, &fdset); // stdin is processed too
		FD_SET (netdata->ip_sockets[NS_SERVER], &fdset); // network socket
		timeout.tv_sec = msec/1000;
		timeout.tv_usec = (msec%1000)*1000;
		select (netdata->ip_sockets[NS_SERVER]+1, &fdset, NULL, NULL, &timeout);
		stdin_ready = FD_ISSET (0, &fdset);
	}
#endif
}

#warning Should return an error o:)
void NET_Init(void)
{
	netdata = malloc(sizeof(*netdata));
	if (netdata)
	{
		memset(netdata, 0, sizeof(*netdata));

		netdata->sysnetdata = Sys_Net_Init();
		if (netdata->sysnetdata == 0)
			Com_Printf("Failed to initialise networking\n");

		// init the message buffer
		SZ_Init(&net_message, net_message_buffer, sizeof(net_message_buffer));
	}
}

void NET_Shutdown (void)
{
	if (netdata == 0)
		Sys_Error("NET_Shutdown() called twice\n");

	NET_ClientConfig(false);
	NET_ServerConfig(false);

	Sys_Net_Shutdown(netdata->sysnetdata);

	free(netdata);
	netdata = 0;
}

