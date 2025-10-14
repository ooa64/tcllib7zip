#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)

#
# Add here whatever m4 macros you want to define for your package
#

AC_DEFUN([TCLSEVENZIP_SETUP_DEBUGLOG], [
    AC_MSG_CHECKING([for build with debuglog])
    AC_ARG_ENABLE(debuglog,
	AS_HELP_STRING([--enable-debuglog],
	    [build with debug printing (default: off)]),
	[tcl_ok=$enableval], [tcl_ok=no])
    if test "$tcl_ok" = "yes"; then
	CFLAGS_DEBUGLOG="-DTCLCMD_DEBUG -DTCLLIB7ZIP_DEBUG -DTCLLIB7ZIPARCHIVE_DEBUG"
	AC_MSG_RESULT([yes])
    else
	CFLAGS_DEBUGLOG=""
	AC_MSG_RESULT([no])
    fi
    AC_SUBST(CFLAGS_DEBUGLOG)
])

AC_DEFUN([TCLSEVENZIP_CHECK_LIB7ZIP], [
    AC_MSG_CHECKING([for lib7zip sources])
    AC_ARG_WITH([lib7zip],
        AS_HELP_STRING([--with-lib7zip=<dir>],
            [path to the lib7zip source directory]
        ), [
            lib7zipdir="$withval"
        ], [
            lib7zipdir="no"
        ]
    )
    AC_MSG_RESULT([$lib7zipdir])

    AC_MSG_CHECKING([for lib7zip/src/lib7zip.h])
    if test "$lib7zipdir" != "no"; then
        if test -f "$lib7zipdir/src/lib7zip.h"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate lib7zip/src/lib7zip.h])
        fi
    else
            AC_MSG_RESULT([skip])
    fi

    AC_MSG_CHECKING([for 7zip source directory])
    AC_ARG_WITH([7zipdir],
        AS_HELP_STRING([--with-7zip=<dir>],
            [path to the 7zip source directory]
        ), [
            7zipdir="$withval"
        ], [
            if test "$lib7zipdir" != "no"; then
                7zipdir="$lib7zipdir/third_party/7zip"
            else
                7zipdir="no"
            fi
        ]
    )
    AC_MSG_RESULT([$7zipdir])

    AC_MSG_CHECKING([for 7zip/C/7zVersion.h])
    if test "$7zipdir" != "no"; then
        if test -f "$7zipdir/C/7zVersion.h"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate 7zip/C/7zVersion.h])
        fi
    else
            AC_MSG_RESULT([skip])
    fi

    if test "$lib7zipdir" != "no"; then
        TEA_ADD_INCLUDES([-I\"`${CYGPATH} $lib7zipdir/src`\"])
    fi
    if test "$lib7zipdir" != "no"; then
        TEA_ADD_LIBS([-L"$lib7zipdir/src"])
    fi
    if test "$7zipdir" != "no"; then
        TEA_ADD_INCLUDES([-I\"`${CYGPATH} $7zipdir`\"])
    fi
])
