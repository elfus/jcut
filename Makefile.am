bin_PROGRAMS = jcut
SUBDIRS = .
jcut_SOURCES = main.cpp TestGeneratorVisitor.cpp TestParser.cpp TestLoggerVisitor.cpp \
    JCUTScanner.cpp linenoise.c utf8.c TestRunnerVisitor.cpp \
    JCUTAction.cpp Interpreter.cpp Interpreter.h JCUTAction.h JCUTScanner.h \
    OSRedirect.h TestGeneratorVisitor.h TestLoggerVisitor.h TestParser.h \
    TestRunnerVisitor.h utf8.h Visitor.h linenoise.h

jcut_CPPFLAGS = -x c++ -std=gnu++11 -fexceptions `llvm-config --cppflags`

jcut_LDADD = $(LLVM_LDADD) -lclangTooling -lclangDriver \
-lclangFrontend -lclangSerialization -lclangCodeGen -lclangParse \
-lclangSema -lclangAnalysis -lclangRewriteFrontend -lclangRewriteCore \
-lclangEdit -lclangAST -lclangLex -lclangBasic $(LLVM_LDADD)
