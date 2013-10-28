bin_PROGRAMS = matedialog

matedialog_SOURCES = \
	about.c		\
	calendar.c	\
	entry.c		\
	fileselection.c	\
	main.c		\
	msg.c		\
	notification.c	\
	option.c	\
	option.h	\
	progress.c	\
	scale.c		\
	text.c		\
	tree.c		\
	color.c         \
	password.c      \
	util.c		\
	util.h		\
	matedialog.h

matedialog_CPPFLAGS = \
	-I$(includedir)					\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMATEDIALOG_DATADIR=\""$(datadir)/matedialog"\"		\
	$(AM_CPPFLAGS)

matedialog_CFLAGS = \
	$(MATEDIALOG_CFLAGS)	\
	$(LIBNOTIFY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)		\
	$(X11_CFLAGS)

matedialog_LDFLAGS = \
	$(AM_LDFLAGS)

matedialog_LDADD = \
	$(MATEDIALOG_LIBS)		\
	$(LIBNOTIFY_LIBS)		\
	$(X11_LIBS)

uidir = $(datadir)/matedialog

ui_DATA = \
	matedialog.ui

EXTRA_DIST = \
	$(ui_DATA)
