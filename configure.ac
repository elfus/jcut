#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([jcut], [1.0], [adrianog.sw@gmail.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([.])
AC_CONFIG_SRCDIR([main.cpp])
# AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AX_CXX_COMPILE_STDCXX_11(,mandatory)

AC_CHECK_PROG(LLVM_CONFIG, llvm-config, [`which llvm-config`], [not-found])
if test "x${LLVM_CONFIG}" = xnot-found; then
   AC_CHECK_PROG(LLVM_CONFIG, llvm-config-3.4, [`which llvm-config-3.4`], [not-found])
   if test "x${LLVM_CONFIG}" = xnot-found; then
      AC_MSG_ERROR(llvm-config 3.4 not found)
   fi
fi

CPPFLAGS=`llvm-config --cppflags`
# Checks for libraries.

AC_SUBST(LLVM_LDADD, $(llvm-config --ldflags  --libs jit interpreter \
nativecodegen bitreader bitwriter irreader ipo linker selectiondag \
asmparser instrumentation option | tr '\n' ' '))


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h sys/ioctl.h termios.h unistd.h])
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([llvm/Support/TargetSelect.h],[],[
	AC_MSG_ERROR("LLVM 3.4 header files are not properly installed!)
])
AC_CHECK_HEADERS([clang/Tooling/Tooling.h], , [
 AC_MSG_ERROR("Clang 3.4 header files are not properly installed!)
])
AC_LANG_POP([C++])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit dup2 memmove memset strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
