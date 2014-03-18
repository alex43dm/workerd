dnl Process this file with autoconf
AC_INIT([workerd], [0.1], [wvdial@gmail.com])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_INSTALL

AC_LANG_CPLUSPLUS

dnl Check for libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])
LTCFLAGS="-O2"
LTCXXFLAGS="-O2"

AC_SUBST([DOLLAR_SIGN],[$])

AC_ARG_ENABLE(dummy,
AS_HELP_STRING([--enable-dummy],
	[enable dummy mode, default: no]),
		[case "${enableval}" in
		yes) dummy=true ;;
		no)  dummy=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-dummy]) ;;
		esac],
		[dummy=false])

AM_CONDITIONAL(DUMMY, test x"$dummy" = x"true")

AC_HAVE_HEADERS(string.h fcntl.h sys/file.h sys/param.h)

if test "x$dummy" = "xfalse" ; then
AC_CONFIG_LIBOBJ_DIR([libs/libredis])
AC_CHECK_LIB([sphinxclient], [sphinx_add_query], [], [AC_MSG_ERROR([sphinxclient was not found])])
else
PACKAGE=workerdDummy
PACKAGE_NAME=workerdDummy
PACKAGE_STRING="workerdDummy 0.1"
PACKAGE_TARNAME=workerdDummy
PACKAGE_VERSION=0.1

AC_DEFINE(DUMMY,[1],[dummy worker])
fi

dnl AC_CONFIG_LIBOBJ_DIR([libs/amqpcpp])

AC_CHECK_LIB([amqpcpp],[main],[],[AC_MSG_ERROR([amqpcpp library missing])])
AC_CHECK_LIB([mongoclient],[main],[],[AC_MSG_ERROR([mongoclient(mongodb build with client shared library) missing])])

AC_LANG(C++)
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -lamqpcpp"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <AMQPcpp.h>],
    [AMQPMessage *m;
    unsigned len;
    char *pMes;
    pMes = m->getMessage(&len);])],
    [],
    [AC_DEFINE(AMQPCPP_OLD,[1],[amqpcpp library old])])

LDFLAGS="$SAVED_LDFLAGS -lmongoclient -lboost_system"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <mongo/client/connpool.h>],
    [mongo::ScopedDbConnection *db;
     db = mongo::ScopedDbConnection::getScopedDbConnection("", 0);])],
     [],
    [AC_DEFINE(MONGO_2_0,[],[mongoclient library old])])

lDFLAGS=$SAVED_LDFLAGS


AC_CHECK_LIB([icuuc],[u_tolower],[],[AC_MSG_WARN([icuuc library missing])])
AC_CHECK_LIB([icui18n],[main],[],[AC_MSG_ERROR([icui18n library missing])])
AC_CHECK_LIB([icudata],[main],[],[AC_MSG_ERROR([icudata library missing])])

BOOST_REQUIRE([1.48],AC_MSG_ERROR([boost>=1.48 required]))
BOOST_FILESYSTEM()
BOOST_THREADS()
BOOST_DATE_TIME()
BOOST_REGEX()

AC_CHECK_LIB([fcgi], [FCGX_FPrintF], [], [AC_MSG_ERROR([fcgi was not found])])
AC_CHECK_LIB([sqlite3], [sqlite3_open], [], [AC_MSG_ERROR([sqlite3 was not found])])
AC_CHECK_LIB([sqlite3], [sqlite3_threadsafe], [AC_DEFINE(SQLITE_THREADSAFE,[1],[sqlite3_threadsafe was found])],[])
AC_CHECK_LIB([GeoIP], [GeoIP_region_name_by_code], [], [AC_MSG_ERROR([GeoIP was not found])])
AC_CHECK_LIB([tinyxml], [main], [], [AC_MSG_ERROR([tinyxml was not found])])
AC_CHECK_LIB([rabbitmq],[main],[],[AC_MSG_ERROR([rabbitmq library missing])])

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
	[enable debugging, default: no]),
		[case "${enableval}" in
		yes) debug=true ;;
		no)  debug=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
		esac],
		[debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

CPPFLAGS="-W -Wall"

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
 Makefile
])

AC_OUTPUT
