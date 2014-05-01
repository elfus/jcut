##===- tools/clang-utest/Makefile --------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

CLANG_LEVEL := ../..

TOOLNAME = jit-testing

SOURCES = main.cpp Visitor.cpp TestParser.cpp

# No plugins, optimize startup time.
TOOL_NO_EXPORTS = 1

CXXFLAGS = -std=c++11 -g -fexceptions -frtti
CFLAGS = -g

include $(CLANG_LEVEL)/../../Makefile.config

LINK_COMPONENTS := $(TARGETS_TO_BUILD) asmparser bitreader support mc option

USEDLIBS = clangTooling.a clangDriver.a clangFrontend.a clangSerialization.a \
	   clangCodeGen.a clangParse.a clangSema.a \
           clangAnalysis.a clangRewriteFrontend.a clangRewriteCore.a \
	   clangEdit.a clangAST.a clangLex.a clangBasic.a

include $(CLANG_LEVEL)/Makefile
