jit-testing
===========

Clang based tool meant to be used for C code unit testing based on LLVM JIT engine

----------
Compiling
==========
This tool assumes is compiled within the clang source tree.

Compiling windows binaries:
When configuring your llvm project issue the following command line:
CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ CPP=i686-w64-mingw32-cpp CXXCPP=i686-w64-mingw32-cpp LDFLAGS="-static -static-libgcc -static-libstdc++" ../llvm34/configure --prefix=$HOME/llvm-win --enable-optimized --enable-doxygen --host=i686-w64-mingw32

This assumes you have installed a cross compiler. Also note the linker flags
(LDFLAGS) are important to link statically to any library the tool may need.
---------
Test File Grammar
========

<test-expr> := <test-file>

<test-file> :=  <test-group>

<global-mockup> := mockup_all "{" <mockup-fixture> "}"

<global-setup> := before_all "{" <test-fixture> "}"

<global-teardown> := after_all "{" <test-fixture> "}"

# Even thought <global-teardown> needs to be defined before a <test-definition>, it will
# actually be executed after the <test-definition>
<test-group> := [<global-mockup> | <global-setup> ] <test-definition>+ [ <global-teardown> ]| group [<identifier>] "{" <test-group>+ "}"

# TODO Right we are forcing the user to use this order, however it would be better
# if we let the use define test-info, test-mockup, and test-setup in whatever order
<test-definition> := [<test-info> | <test-mockup> | <test-setup>] <test-function> [<test-teardown>]

<test-info> := test <identifier>

<test-mockup> := mockup "{" <mockup-fixture> "}"

<test-setup> :=  before "{"  <test-fixture> "}"

<test-function> := <function-call> [<expected-result>] ;


<test-teardown> := after "{" <test-fixture> "}"

<test-fixture> := <function-call>;* | <var-assignment>;* | <expected-expression>;*

<mockup-fixture> := <mockup-function>;* | <mockup-variable>;*

<mockup-function> :=  <function-call> = <argument>

<mockup-variable> := <var-assignment>

<expected-result> :=  <comparison-operator> <expected-constant> 

# We may want to remove constant and just put NumericConstant and CharConstant
# for ExpectedConstant
<expected-constant> := <constant>
<constant> := <numeric-constant> | <string-constant>| <char-constant>
<numeric-constant> := [-][<integer>|<float>] 
<string-constant> := string (the usual enclosed string with quotes like this "string" )
<char-constant> := char ( a single character with single quote like this 'a' )

<expected-expression> := <operand> <comparison-operator> <operand>
<operand> := <constant> | <identifier>

<comparison-operator> := "==" | "!=" | ">=" | "<=" | "<" | ">"

# Until here
<var-assignment> := <identifier> = <argument> | <struct-initializer> | <buffer-alloc>

<function-call> := <function-name>"(" <function-argument>* ")"

<function-argument> := {<argument> | <buffer-alloc>}

<argument> := [-]<number> | string |  char | <array-initializer>

<number> := int | double |  float

<buffer-alloc> := "[" int [":" [int | <struct-initializer> ]] "]"

<array-initializer> := { <number>+ }

<struct-initializer> := { <initializer-list> | <designated-initializer> }

<intializer-list> := (<initializer-value> [,])+

<designated-initializer> := ( .<identifier> = <initializer-value> [,])+

<initializer-value> := <argument> | <struct-initializer> 

<function-name> := <identifier>

<identifier> := identifier string
