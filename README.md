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

	adrianog.sw@gmail.com

You can download the source tarball here:

https://www.dropbox.com/sh/n9znjt4r21c0f30/AAAKt6H5BHDU2Lx0Q0KxJlXva?dl=0

And the source code can be directly downloaded from:

https://github.com/elfus/jcut

Thanks for using this tool!

Compiling jcut from source
-----------
You have two options to compile jcut from source, the easy and the hard one:

  1. Easy way: install jcut using the well known ./configure script.
  2. Hard way: install jcut by compiling it within the clang source tree.

These options are explained below:
>>>>>
  1. Easy way: install jcut using the well known ./configure script.

Before you attempt to ./configure and make jcut you need to install its
dependencies.

JCUT depends on the llvm 3.4 and clang 3.4 libraries. And in turn you need
to satisfy their own dependencies. An easy way to install such dependencies
is by using the package manager provided in your Linux distro.

This tarball has been tested under Linux Mint 17, Ubuntu 14.04 and Fedora 20.
For Fedora you can install all the dependencies with the following command:

  sudo yum install zlib-devel gcc-c++ llvm-3.4 llvm-devel-3.4 llvm-static-3.4 clang-devel-3.4 

For Linux Mint 17 and Ubuntu 14.04 you can install al the dependencies with
the following command:

  sudo apt-get install g++ llvm llvm-3.4 llvm-3.4-runtime llvm-3.4-dev clang-3.4 libclang-3.4 libclang-3.4-dev

All other linux distros will have to install their equivalent packages to the
package names from the previous two command lines. Keep in mind that you may
need to install dependencies needed by either LLVM or Clang.

JCUT is being compiled statically, so you need to install both the shared and
static libraries. The above commands will do the trick for Fedora, Linux Mint
and Ubuntu. This will change in time as this the first 'autotool' package
done by Adrian, it will take some time while he gets familiar with it to
tweak it.

After installing the dependencies inside the jcut-X.X.tar.gz file there 
is a script called 'configure'. Now you can execute:

  ./configure && make
  make install # If installing in a system path, otherwise just use make

NOTE: It is highly recommended to build JCUT in its own build directory. Just
create a another diferent folder anywher you want and execute the configure
script inside that folder. This will keep the original JCUT folder clean and
you can easily hack it without the noise of having lots of object files.

Another advantage of creating a different directory to 'configure' it is that
the Makefile that jcut is distributed with is used to compile it within the
Clang source tree. See section '2. Hard way'

See the INSTALL file for more information on how to use the 'configure' script.

>>>>>
  2. Hard way: install jcut by compiling it within the clang source tree.

The Makefile included with JCUT contains all the rules needed to compile JCUT
within the Clang source tree. If you happen to choose option 1 for compiling
JCUT and then you 'configure' it in the same source directory, the 'configure'
script will overwrite such Makefile and you will not be able to compile
JCUT from within the Clang source tree again. The solution to this is download
the Makefile again and if you 'configure' it as described in option 1 make sure
you create a build directory different from the source directory.

The LLVM and clang version used to develop this tool is 3.4! Once there is
an official release for 3.5 the porting to such version will be done :)

Follow the steps here

http://clang.llvm.org/get_started.html

And right before step number 6 make sure you clone the jcut folder in
the following location:

cd llvm/tools/clang/tools
git clone https://github.com/elfus/jcut.git

Then modify the Makefile located at:

llvm/tools/clang/tools/Makefile

To add the jcut folder as part of the compilation process, i.e. I
added this to end of the Makefile before include directive.

...
DIRS += jcut

include $(CLANG_LEVEL)/Makefile
<eof>

Continue with the step number 6 from the clang website.



