
GAIM_TOP :=		$(GPP_TOP)/../../..
GTK_TOP :=		$(GAIM_TOP)/../win32-dev/gtk_2_0
GAIM_INSTALL_DIR :=	$(GAIM_TOP)/win32-install-dir
DLL_INSTALL_DIR :=	$(GAIM_INSTALL_DIR)/plugins
DLL_ZIP_DIR :=		$(GPP_TOP)/win32-dist

GPP_VERSION := $(shell cat ${GPP_TOP}/VERSION)
GPP_CONFIG_H := $(GPP_TOP)/gpp_config.h

include $(GAIM_TOP)/libgaim/win32/global.mak

DEFINES += -DGPP_VERSION=\"$(GPP_VERSION)\"

##
## INCLUDE PATHS
##

INCLUDE_PATHS +=	-I. \
			-I$(GTK_TOP)/include \
			-I$(GTK_TOP)/include/gtk-2.0 \
			-I$(GTK_TOP)/include/glib-2.0 \
			-I$(GTK_TOP)/include/pango-1.0 \
			-I$(GTK_TOP)/include/atk-1.0 \
			-I$(GTK_TOP)/include/freetype2 \
			-I$(GTK_TOP)/lib/glib-2.0/include \
			-I$(GTK_TOP)/lib/gtk-2.0/include \
			-I$(GAIM_TOP)/libgaim \
			-I$(GAIM_TOP)/libgaim/win32 \
			-I$(GAIM_TOP)/gtk \
			-I$(GAIM_TOP)/gtk/win32 \
			-I$(GAIM_TOP)


LIB_PATHS =		\
			-L$(GTK_TOP)/lib \
			-L$(GAIM_TOP)/libgaim \
			-L$(GAIM_TOP)/gtk

##
##  SOURCES, OBJECTS
##

GPP_SRC ?= $(GPP).c


GPP_OBJ = $(GPP_SRC:%.c=%.o)

##
## LIBRARIES
##

PLUGIN_LIBS = \
	-lgtk-win32-2.0 \
	-lgdk-win32-2.0 \
	-lglib-2.0 \
	-lgmodule-2.0 \
	-lgobject-2.0 \
	-lws2_32 \
	-lintl \
	-llibgaim \
	-lgtkgaim

##
## RULES
##

# How to make a C file
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(DEFINES) -c $< -o $@

##
## TARGET DEFINITIONS
##


.PHONY: all clean install install_zip

all: $(GPP).dll

$(GPP_CONFIG_H): $(GPP_TOP)/gpp_config.h.mingw
	cp $(GPP_TOP)/gpp_config.h.mingw $(GPP_CONFIG_H)

$(DLL_ZIP_DIR):
	mkdir $(DLL_ZIP_DIR)

install: all
	cp $(GPP).dll $(DLL_INSTALL_DIR)

install_zip: $(DLL_ZIP_DIR) all
	cp $(GPP).dll $(DLL_ZIP_DIR)

$(GPP_OBJ): $(GPP_CONFIG_H)

##
## BUILD DLL
##

$(GPP).dll: $(GPP_OBJ) $(GAIM_TOP)/libgaim/libgaim.dll.a $(GAIM_TOP)/gtk/gtkgaim.dll.a
	$(CC) -shared $(GPP_OBJ) $(LIB_PATHS) $(PLUGIN_LIBS) $(DLL_LD_FLAGS) -o $(GPP).dll


##
## CLEAN RULES
##

clean:
	rm -rf *.o
	rm -rf $(GPP).dll


