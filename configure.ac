AC_PREREQ([2.69])

AC_INIT(	[ecotube],
		[25.7.3],
		[https://github.com/ecotubehq/player/issues],
		[ecotube],
		[https://github.com/ecotubehq/player] )

AC_CONFIG_SRCDIR([src/celluloid-main.c])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([1.12 tar-pax dist-xz no-dist-gzip subdir-objects foreign -Wall -Wno-portability])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

# i18n
GETTEXT_PACKAGE=celluloid
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

# TODO: Remove AM_GNU_GETTEXT_VERSION once autoreconf supports
# AM_GNU_GETTEXT_REQUIRE_VERSION
AM_GNU_GETTEXT_VERSION([0.19])
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19])

AM_GNU_GETTEXT([external])

GLIB_GSETTINGS

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging]), [enable_debug=$enableval], [enable_debug=no])

AC_DEFUN([GM_CHECK_MACRO],
	[m4_ifndef([$1], [m4_fatal($1[ is not defined. Is ]$2[ installed?])])])

GM_CHECK_MACRO([AX_APPEND_COMPILE_FLAGS], [autoconf-archive])
GM_CHECK_MACRO([AX_APPEND_LINK_FLAGS], [autoconf-archive])
GM_CHECK_MACRO([AX_REQUIRE_DEFINED], [autoconf-archive])

GM_CHECK_MACRO([APPSTREAM_XML], [appstream-glib])
APPSTREAM_XML

AS_IF([test "x$enable_debug" = "xyes"], [
	AX_APPEND_COMPILE_FLAGS([-O0 -g])
], [
	AX_APPEND_COMPILE_FLAGS([-O2])
])

AX_APPEND_COMPILE_FLAGS([ \
	-std=gnu99 \
	-funsigned-char \
	-fstack-protector-strong \
	-fPIE \
	-fPIC \
	-Wall \
	-Wextra \
	-Wconversion \
	-Winline \
	-Wno-padded \
	-Wno-unused-parameter \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Werror=implicit-function-declaration \
	-Werror=pointer-arith \
	-Werror=init-self \
	-Werror=format-security \
	-Werror=format=2 \
	-Werror=missing-include-dirs \
	-Werror=date-time \
])

AX_APPEND_LINK_FLAGS([ \
	-pie \
	-Wl,-z,relro \
	-Wl,-z,now \
])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_AWK
AC_PROG_SED
AC_PATH_PROG(GDBUS_CODEGEN, [gdbus-codegen], [])
AC_PATH_PROG(GLIB_GENMARSHAL, [glib-genmarshal], [])

AS_IF([test "x$GDBUS_CODEGEN" = "x"], [
	AC_MSG_ERROR([Could not find gdbus-codegen])
])

AS_IF([test "x$GLIB_GENMARSHAL" = "x"], [
	AC_MSG_ERROR([Could not find glib-genmarshal])
])

PKG_CHECK_MODULES(DEPS, [gtk4 >= 4.6.1 glib-2.0 >= 2.66 libadwaita-1 >= 1.5.0 mpv >= 1.107 epoxy])
AC_SEARCH_LIBS([sqrt], [m])

AC_DEFINE([GLIB_VERSION_MIN_REQUIRED], [GLIB_VERSION_2_66], [Dont warn using older APIs])
AC_DEFINE([GLIB_VERSION_MAX_ALLOWED], [GLIB_VERSION_2_66], [Prevents using newer APIs])

AC_DEFINE([GDK_VERSION_MIN_REQUIRED], [GDK_VERSION_4_6], [Dont warn using older APIs])
AC_DEFINE([GDK_VERSION_MAX_ALLOWED], [GDK_VERSION_4_6], [Prevents using newer APIs])

AC_DEFINE_UNQUOTED([G_LOG_DOMAIN], "$PACKAGE_NAME", [Default logging facility])

# Checks for header files.
AC_CHECK_HEADERS([string.h stdio.h stdlib.h locale.h],[],[
	AC_MSG_ERROR([Could not find required headers])
])

AC_CHECK_FUNCS([setlocale],[],[
	AC_MSG_ERROR([Could not find required functions])
])

AC_CONFIG_FILES([Makefile src/Makefile data/Makefile docs/Makefile po/Makefile.in])
AC_OUTPUT

echo "
################################################################################
#    THE AUTOMAKE BUILD SCRIPT IS DEPRECATED. CONSIDER USING MESON INSTEAD.    #
################################################################################

	$PACKAGE $VERSION

	prefix ........: $prefix
	debug .........: $enable_debug
"
