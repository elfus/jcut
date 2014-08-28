jcut
===========

jcut is a simple command line tool to perform unit testing in C code
using the LLVM Just In Time (JIT) engine and some of the Clang APIs.

Under the folder 'tutorial' you will find a PDF/txt file with a small tutorial
covering the usage and features of the jcut tool. In there you will
find an example C file and an example test file.

jcut is open source software. You may freely distribute it under the
terms of the license agreement found in LICENSE.txt.

jcut is in its really early stages of life, so any feedback or bug
report can be sent to:

	adrian.ortega.sweng@gmail.com

Thanks for using this tool!

Compiling.
-----------
The LLVM and clang version used to develop this tool is 3.4! Once there is
an official release for 3.5 the porting to such version will be done :)

This project has to be compiled as part of the clang source tree. Follow
the steps here

http://clang.llvm.org/get_started.html

And right before step number 6 make sure you clone the jit-testing folder in
the following location:

cd llvm/tools/clang/tools
git clone https://github.com/elfus/jcut.git

Then modify the Makefile

llvm/tools/clang/tools/Makefile

To add the jcut folder as part of the compilation process, i.e. I
added this to end of the Makefile before include directive.

...
DIRS += jcut

include $(CLANG_LEVEL)/Makefile
<eof>



