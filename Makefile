
VPATH=../../../

CC=gcc
STRIP=strip

CFLAGS=-O2 -g -Wall -fno-strict-aliasing -DNETQW $(OSCFLAGS) $(CPUCFLAGS) $(RENDERERCFLAGS)
STRIPFLAGS=--strip-unneeded --remove-section=.comment

TARGETSYSTEM:=$(shell $(CC) -dumpmachine)

OS=$(shell echo $(TARGETSYSTEM) | sed "s/-gnu//" | sed "s/.*-//" | tr [A-Z] [a-z] | sed s/^mingw.*/win32/)
CPU=$(shell echo $(TARGETSYSTEM) | cut -d '-' -f 1 | tr [A-Z] [a-z] | sed "s/powerpc/ppc/")

TARGETS=sw gl

# OS specific settings

ifeq ($(OS), morphos)
	OSCFLAGS=-noixemul -D__MORPHOS_SHAREDLIBS
	OSOBJS= \
		sys_morphos.o \
		net_amitcp.o \
		thread_morphos.o \
		cd_morphos.o \
		snd_morphos.o \
		in_morphos.o

	OSSWOBJS=vid_mode_morphos.o vid_morphos.o

	OSGLOBJS=vid_mode_morphos.o vid_tinygl.o
endif

ifeq ($(OS), linux)
	OSOBJS= \
		sys_linux.o \
		sys_error_gtk.o \
		net_posix.o \
		thread_posix.o \
		cd_linux.o \
		snd_oss.o \
		snd_alsa.o

	OSCFLAGS=-DBUILD_STRL
	OSLDFLAGS=-lpthread -lrt

	OSSWOBJS=vid_x11.o vid_mode_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga

	OSGLOBJS=vid_glx.o vid_mode_x11.o in_x11.o
	OSGLLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga -lGL
endif

ifeq ($(OS), freebsd)
	OSOBJS= \
		sys_linux.o \
		cd_null.o \
		snd_oss.o

	OSCFLAGS=-I/usr/X11R6/include

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga

	OSGLOBJS=vid_glx.o in_x11.o
	OSGLLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga -lGL
endif

ifeq ($(OS), netbsd)
	OSOBJS= \
		sys_linux.o \
		cd_null.o \
		snd_oss.o

	OSCFLAGS=-I/usr/X11R6/include
	OSLDFLAGS=-lossaudio

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga

	OSGLOBJS=vid_glx.o in_x11.o
	OSGLLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga -lGL
endif

ifeq ($(OS), openbsd)
	OSOBJS= \
		sys_linux.o \
		cd_null.o \
#		snd_oss.o

	OSCFLAGS=-I/usr/X11R6/include
	OSLDFLAGS=-lossaudio

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga

	OSGLOBJS=vid_glx.o in_x11.o
	OSGLLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga -lGL
endif

ifeq ($(OS), cygwin)
	OSOBJS= \
		sys_linux.o \
		cd_null.o \
		snd_null.o

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext
endif

ifeq ($(OS), win32)
	OSOBJS= \
		sys_win.o \
		thread_win32.o \
		snd_dx7.o \
		net_win32.o \
		cd_null.o \
		in_dinput8.o \

	OSSWOBJS=vid_win.o vid_mode_win32.o
	OSSWLDFLAGS=-lmgllt -lwsock32 -lgdi32 -ldxguid -lwinmm

	OSGLOBJS=vid_wgl.o vid_mode_win32.o
	OSGLLDFLAGS=-lopengl32 -lwinmm -lwsock32 -lgdi32 -ldxguid -ldinput8

	OSCFLAGS = -I`cd ~/directx && pwd` -DBUILD_STRL
	ifneq ($(shell $(CC) -dumpmachine | grep cygwin),)
		OSCFLAGS+= -mno-cygwin
	endif
	OSLDFLAGS = -mwindows
endif

ifeq ($(OS), gekko)
# Hey, we're actually libogc on the Wii.

	OSOBJS = \
		sys_wii.o \
		net_null.o \
		cd_null.o \
		fatfs/libfat.a
	
	OSSWOBJS = vid_wii.o in_wii.o

	# -mrvl
	# -I/usr/local/devkitPro/devkitPPC/powerpc-gekko/include/
	OSCFLAGS = -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float -I/usr/local/devkitPro/devkitPPC/include
	OSLDFLAGS = -L/usr/local/devkitPro/devkitPPC/lib/wii/ -logc
endif

# CPU specific settings

ifeq ($(CPU), ppc)
	CPUCFLAGS=-DFOD_BIGENDIAN
endif

OBJS= \
	cl_auth.o \
	cl_sbar.o \
	cl_screen.o \
	cl_cam.o \
	cl_capture.o \
	cl_cmd.o \
	cl_demo.o \
	cl_ents.o \
	cl_fchecks.o \
	cl_fragmsgs.o \
	cl_ignore.o \
	cl_input.o \
	cl_logging.o \
	cl_main.o \
	cl_parse.o \
	cl_pred.o \
	cl_tent.o \
	cl_view.o \
	cmd.o \
	com_msg.o \
	common.o \
	config_manager.o \
	console.o \
	crc.o \
	cvar.o \
	filesystem.o \
	fmod.o \
	host.o \
	huffman.o \
	image.o \
	keys.o \
	linked_list.o \
	match_tools.o \
	mathlib.o \
	md5.o \
	mdfour.o \
	menu.o \
	modules.o \
	mouse.o \
	mp3_player.o \
	net_chan.o \
	net.o \
	netqw.o \
	pmove.o \
	pmovetst.o \
	pr_edict.o \
	pr_exec.o \
	pr_cmds.o \
	r_part.o \
	readablechars.o \
	ruleset.o \
	server_browser.o \
	serverscanner.o \
	skin.o \
	sleep.o \
	snd_main.o \
	snd_mem.o \
	snd_mix.o \
	strlcat.o \
	strlcpy.o \
	sv_ccmds.o \
	sv_ents.o \
	sv_init.o \
	sv_main.o \
	sv_move.o \
	sv_nchan.o \
	sv_phys.o \
	sv_save.o \
	sv_send.o \
	sv_user.o \
	sv_world.o \
	teamplay.o \
	utils.o \
	version.o \
	vid.o \
	wad.o \
	zone.o \
	$(OSOBJS)

SWOBJS= \
	d_edge.o \
	d_init.o \
	d_modech.o \
	d_polyse.o \
	d_scan.o \
	d_sky.o \
	d_sprite.o \
	d_surf.o \
	d_vars.o \
	d_zpoint.o \
	r_aclip.o \
	r_alias.o \
	r_bsp.o \
	r_draw.o \
	r_edge.o \
	r_efrag.o \
	r_light.o \
	r_main.o \
	r_model.o \
	r_misc.o \
	r_sky.o \
	r_sprite.o \
	r_surf.o \
	r_rast.o \
	r_vars.o \
	$(OSSWOBJS)

GLOBJS= \
	gl_draw.o \
	gl_mesh.o \
	gl_model.o \
	gl_ngraph.o \
	gl_refrag.o \
	gl_rlight.o \
	gl_rmain.o \
	gl_rmisc.o \
	gl_rpart.o \
	gl_rsurf.o \
	gl_texture.o \
	gl_warp.o \
	vid_common_gl.o \
	$(OSGLOBJS)

all: $(TARGETS) compilercheck

gl:
	mkdir -p objects/$(TARGETSYSTEM)/gl
	(cd objects/$(TARGETSYSTEM)/gl; $(MAKE) -f $(VPATH)Makefile fodquake-gl RENDERERCFLAGS=-DGLQUAKE)

sw:
	mkdir -p objects/$(TARGETSYSTEM)/sw
	(cd objects/$(TARGETSYSTEM)/sw; $(MAKE) -f $(VPATH)Makefile fodquake-sw)


clean:
	rm -rf objects

fodquake-sw: $(OBJS) $(SWOBJS)
	$(CC) $(CFLAGS) $^ -lm $(OSLDFLAGS) $(OSSWLDFLAGS) -o $@.db
	$(STRIP) $(STRIPFLAGS) $@.db -o $@

fodquake-gl: $(OBJS) $(GLOBJS)
	$(CC) $(CFLAGS) $^ -lm $(OSLDFLAGS) $(OSGLLDFLAGS) -o $@.db
	$(STRIP) $(STRIPFLAGS) $@.db -o $@

sys_error_gtk.o: CFLAGS+=`pkg-config --cflags gtk+-2.0`

compilercheck:
# Check for GCC 4

	@if [ ! -z "`$(CC) -v 2>&1 | grep \"gcc version 4\"`" ]; \
	then \
		echo ""; \
		echo ""; \
		echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"; \
		echo "GCC version 4 was detected."; \
		echo ""; \
		echo "Please note that builds done with GCC 4 or newer are not supported. If you"; \
		echo "experience any problems with Fodquake built with this compiler, please either"; \
		echo "disable optimisations or build Fodquake with GCC 2.95.3, GCC 3.3.6 or GCC"; \
		echo "3.4.6."; \
		echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"; \
		echo ""; \
		echo ""; \
	fi

.PHONY: compilercheck

