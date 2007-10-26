
VPATH=../../../

CC=gcc
STRIP=strip

CFLAGS=-O2 -Wall -fno-strict-aliasing $(OSCFLAGS) $(CPUCFLAGS) $(RENDERERCFLAGS)
STRIPFLAGS=--strip-unneeded --remove-section=.comment

TARGETSYSTEM?=$(shell $(CC) -dumpmachine)

OS=$(shell echo $(TARGETSYSTEM) | sed "s/-gnu//" | sed "s/.*-//" | tr [A-Z] [a-z] | sed s/^mingw.*/win32/)
CPU=$(shell echo $(TARGETSYSTEM) | cut -d '-' -f 1 | tr [A-Z] [a-z] | sed "s/powerpc/ppc/")

# OS specific settings

ifeq ($(OS), morphos)
	OSCFLAGS=-noixemul
	OSOBJS=sys_morphos.o cd_morphos.o snd_morphos.o in_morphos.o
	OSLDFLAGS=-lsyscall

	OSSWOBJS=vid_morphos.o

	OSGLOBJS=vid_tinygl.o
endif

ifeq ($(OS), linux)
	OSOBJS= \
		sys_linux.o \
		cd_linux.o \
		snd_oss.o

	OSCFLAGS=-DBUILD_STRL

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lXxf86dga

	OSGLOBJS=vid_glx.o in_x11.o
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
		snd_dx7.o \
		cd_win.o \
		in_win.o \

	OSSWOBJS=vid_win.o
	OSSWLDFLAGS=-lmgllt -lwsock32 -lgdi32 -ldxguid -lwinmm

	OSGLOBJS=vid_wgl.o
	OSGLLDFLAGS=-lopengl32 -lwinmm -lwsock32 -lgdi32 -ldxguid

	OSCFLAGS = -I$(VPATH)directx -DBUILD_STRL
	ifneq ($(shell $(CC) -dumpmachine | grep cygwin),)
		OSCFLAGS+= -mno-cygwin
	endif
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
	cl_slist.o \
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
	match_tools.o \
	mathlib.o \
	mdfour.o \
	menu.o \
	modules.o \
	mp3_player.o \
	net_chan.o \
	net_udp.o \
	pmove.o \
	pmovetst.o \
	pr_edict.o \
	pr_exec.o \
	pr_cmds.o \
	r_part.o \
	rulesets.o \
	skin.o \
	sleep.o \
	snd_main.o \
	snd_mem.o \
	snd_mix.o \
	strlcat.o \
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
	
all: compilercheck
	@echo $(TARGETSYSTEM)
	@echo OS: $(OS)
	@echo CPU: $(CPU)

	mkdir -p objects/$(TARGETSYSTEM)/sw
	(cd objects/$(TARGETSYSTEM)/sw; $(MAKE) -f $(VPATH)Makefile fodquake-sw)

	mkdir -p objects/$(TARGETSYSTEM)/gl
	(cd objects/$(TARGETSYSTEM)/gl; $(MAKE) -f $(VPATH)Makefile fodquake-gl RENDERERCFLAGS=-DGLQUAKE)


clean:
	rm -rf objects

fodquake-sw: $(OBJS) $(SWOBJS)
	$(CC) $(CFLAGS) $^ -lm $(OSLDFLAGS) $(OSSWLDFLAGS) -o $@.db
	$(STRIP) $(STRIPFLAGS) $@.db -o $@

fodquake-gl: $(OBJS) $(GLOBJS)
	$(CC) $(CFLAGS) $^ -lm $(OSLDFLAGS) $(OSGLLDFLAGS) -o $@.db
	$(STRIP) $(STRIPFLAGS) $@.db -o $@

compilercheck:
# Check for GCC 4

	@if [ ! -z "`$(CC) -v 2>&1 | grep \"gcc version 4\"`" ]; \
	then \
		echo "GCC version 4 is not supported because it generates broken code. Please use an older (and working) version of GCC."; \
		exit 1; \
	fi

