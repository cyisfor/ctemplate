AC_ARG_ENABLE(debug,
AS_HELP_STRING([--disable-debug],
               [disable debugging, default: no]),
[case "${enableval}" in
             yes) debug=false ;;
             no)  debug=true ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --disable-debug]) ;;
esac],
[debug=true])

AC_DEFUN([ADD_GCC_OPTIMIZATION], [
AC_MSG_CHECKING(optimization)
AS_CASE([$CFLAGS],
	[*-g*], [AC_MSG_RESULT(user specified debugging detected.)],
	[*-O*], [AC_MSG_RESULT(user specified optimization detected.)],
	AS_IF([$debug],
		[CFLAGS="${CFLAGS} -ggdb"],
		[CFLAGS="${CFLAGS} -O2"]))
		])
	
