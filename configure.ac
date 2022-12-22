#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT([prerex],[6.8.0],[rdt@cs.queensu.ca])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

AC_CHECK_LIB([edit], [el_push], [],
  [AC_CHECK_LIB([readline], [readline], [], [AC_MSG_ERROR([libreadline or libedit is required!])])])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([readline/readline.h readline/history.h])
AC_CHECK_HEADERS([editline/readline.h histedit.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T


# Checks for library functions.
AC_CHECK_FUNCS([strstr])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
