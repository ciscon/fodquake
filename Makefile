
VPATH=../../../

CC=gcc
STRIP=strip

CFLAGS=-O2 -Wall -fno-strict-aliasing $(OSCFLAGS) $(CPUCFLAGS) $(RENDERERCFLAGS)
STRIPFLAGS=--strip-unneeded --remove-section=.comment

TARGETSYSTEM?=$(shell $(CC) -dumpmachine)

OS=$(shell echo $(TARGETSYSTEM) | cut -d '-' -f 2 | tr [A-Z] [a-z])
CPU=$(shell echo $(TARGETSYSTEM) | cut -d '-' -f 1 | tr [A-Z] [a-z])

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
		snd_linux.o

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm
endif

ifeq ($(OS), cygwin)
	OSOBJS= \
		sys_linux.o \
		cd_null.o \
		snd_null.o

	OSSWOBJS=vid_x11.o in_x11.o
	OSSWLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext
endif

ifeq ($(OS), mingw32)
	OSOBJS= \
		sys_win.o \
		snd_win.o \
		cd_win.o \
		in_win.o \

	OSSWOBJS=vid_win.o
	OSSWLDFLAGS=-lmgllt -lwsock32 -lgdi32 -ldxguid -lwinmm
endif

# CPU specific settings

ifeq ($(CPU), ppc)
	CPUCFLAGS=-DBIGENDIAN
endif

OBJS= \
	host.o \
	snd_main.o \
	snd_mem.o \
	snd_mix.o \
	cl_input.o \
	keys.o \
	net_chan.o \
	net_udp.o \
	pr_exec.o \
	pr_edict.o \
	pr_cmds.o \
	pmove.o \
	pmovetst.o \
	r_part.o \
	sv_ccmds.o \
	sv_save.o \
	sv_ents.o \
	sv_init.o \
	sv_main.o \
	sv_move.o \
	sv_nchan.o \
	sv_phys.o \
	sv_send.o \
	sv_user.o \
	sv_world.o \
	cl_auth.o \
	cl_cam.o \
	cl_capture.o \
	cl_cmd.o \
	cl_demo.o \
	cl_ents.o \
	cl_fchecks.o \
	cl_fragmsgs.o \
	cl_ignore.o \
	cl_logging.o \
	cl_main.o \
	cl_parse.o \
	cl_pred.o \
	cl_slist.o \
	cl_tent.o \
	cl_view.o \
	cmd.o \
	common.o \
	com_msg.o \
	console.o \
	crc.o \
	cvar.o \
	image.o \
	mathlib.o \
	mdfour.o \
	menu.o \
	cl_sbar.o \
	cl_screen.o \
	skin.o \
	teamplay.o \
	version.o \
	wad.o \
	zone.o \
	modules.o \
	match_tools.o \
	utils.o \
	rulesets.o \
	config_manager.o \
	mp3_player.o \
	fmod.o \
	vid.o \
	$(OSOBJS)

SWOBJS= \
	d_edge.o \
	d_fill.o \
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
	
all:
	@echo $(TARGETSYSTEM)
	@echo OS: $(OS)
	@echo CPU: $(CPU)

	mkdir -p objects/$(TARGETSYSTEM)/sw
	(cd objects/$(TARGETSYSTEM)/sw; make -f $(VPATH)Makefile fodquake-sw)

	mkdir -p objects/$(TARGETSYSTEM)/gl
	(cd objects/$(TARGETSYSTEM)/gl; make -f $(VPATH)Makefile fodquake-gl RENDERERCFLAGS=-DGLQUAKE)


clean:
	rm -rf objects

release: fodquake-sw fodquake-gl

fodquake-sw: $(OBJS) $(SWOBJS)
	$(CC) $(CFLAGS) $^ -lm $(OSLDFLAGS) $(OSSWLDFLAGS) -o $@.db
	$(STRIP) $(STRIPFLAGS) $@.db -o $@

fodquake-gl: $(OBJS) $(GLOBJS)
	$(CC) $(CFLAGS) $^ -lm $(OSLDFLAGS) $(OSGLLDFLAGS) -o $@.db
	$(STRIP) $(STRIPFLAGS) $@.db -o $@
	
