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

SOURCES = main.cpp TestGeneratorVisitor.cpp TestParser.cpp TestLoggerVisitor.cpp

# No plugins, optimize startup time.
TOOL_NO_EXPORTS = 1

USEDLIBS = clangTooling.a clangDriver.a clangFrontend.a clangSerialization.a \
	   clangCodeGen.a clangParse.a clangSema.a \
           clangAnalysis.a clangRewriteFrontend.a clangRewriteCore.a \
	   clangEdit.a clangAST.a clangLex.a clangBasic.a

LINK_COMPONENTS := $(TARGETS_TO_BUILD) jit interpreter nativecodegen bitreader bitwriter irreader \
	ipo linker selectiondag asmparser instrumentation option


include $(CLANG_LEVEL)/../../Makefile.config

include $(CLANG_LEVEL)/Makefile

CXXFLAGS = -std=c++11 -ggdb3 -fexceptions -frtti
CFLAGS = -ggdb3
