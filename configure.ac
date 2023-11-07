AC_INIT(configure.in, 1.0)

AC_CANONICAL_HOST


AC_PROG_CXX(cxx CC g++)


AM_INIT_AUTOMAKE (rcdaq,1.00)

AC_PROG_INSTALL

AC_PROG_LIBTOOL

AC_CHECK_HEADER([rpc/rpc.h], [ ],
                [AC_SUBST(CPPFLAGS, "-I/usr/include/tirpc")] [AC_SUBST(RPC_LDD, "-ltirpc")])

AC_CHECK_HEADERS([mosquitto.h])

AC_CHECK_HEADER([mosquitto.h], [enable_mqtt=yes ],[ ])

AM_CONDITIONAL([MOSQUITTO], [test "x$enable_mqtt" = "xyes"])


dnl Example of default-disabled feature
AC_ARG_ENABLE([WRITEPRDF],
    AS_HELP_STRING([--enable-WRITEPRDF], [compile for legacy PRDF version]))

AS_IF([test "x$enable_WRITEPRDF" = "xyes"], [AC_DEFINE_UNQUOTED([WRITEPRDF],[1])])



AM_CXXFLAGS=" -g -O2"


AC_OUTPUT(Makefile)

