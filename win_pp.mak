-include $(PP_TOP)/local.mak

PIDGIN_TREE_TOP ?=	$(PP_TOP)/../../..
GTK_TOP :=		$(PIDGIN_TREE_TOP)/../win32-dev/gtk_2_0
DLL_ZIP_DIR :=		$(PP_TOP)/win32-dist

PP_VERSION := $(shell cat ${PP_TOP}/VERSION)
PP_CONFIG_H := $(PP_TOP)/pp_config.h

include $(PIDGIN_TREE_TOP)/libpurple/win32/global.mak

DEFINES += -DPP_VERSION=\"$(PP_VERSION)\"

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
			-I$(PURPLE_TOP) \
			-I$(PURPLE_TOP)/win32 \
			-I$(PIDGIN_TOP) \
			-I$(PIDGIN_TOP)/win32 \
			-I$(PIDGIN_TREE_TOP)


LIB_PATHS =		\
			-L$(GTK_TOP)/lib \
			-L$(PURPLE_TOP) \
			-L$(PIDGIN_TOP)

##
##  SOURCES, OBJECTS
##

PP_SRC ?= $(PP).c


PP_OBJ = $(PP_SRC:%.c=%.o)

##
## LIBRARIES
##

PLUGIN_LIBS = \
	-lgtk-win32-2.0 \
	-lgdk-win32-2.0 \
	-lgdk_pixbuf-2.0 \
	-lglib-2.0 \
	-lpango-1.0 \
	-lgmodule-2.0 \
	-lgobject-2.0 \
	-lws2_32 \
	-lintl \
	-lpurple \
	-lpidgin

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

all: $(PP).dll

$(PP_CONFIG_H): $(PP_TOP)/pp_config.h.mingw
	cp $(PP_TOP)/pp_config.h.mingw $(PP_CONFIG_H)

$(DLL_ZIP_DIR):
	mkdir -p $(DLL_ZIP_DIR)

install: all $(PIDGIN_INSTALL_PLUGINS_DIR)
	cp $(PP).dll $(PIDGIN_INSTALL_PLUGINS_DIR)

install_zip: $(DLL_ZIP_DIR) all
	cp $(PP).dll $(DLL_ZIP_DIR)

$(PP_OBJ): $(PP_CONFIG_H) $(PURPLE_VERSION_H)

##
## BUILD DLL
##

$(PP).dll: $(PP_OBJ) $(PURPLE_DLL).a $(PIDGIN_DLL).a
	$(CC) -shared $(PP_OBJ) $(LIB_PATHS) $(PLUGIN_LIBS) $(DLL_LD_FLAGS) -o $(PP).dll


##
## CLEAN RULES
##

clean:
	rm -rf *.o
	rm -rf $(PP).dll

include $(PIDGIN_COMMON_TARGETS)
