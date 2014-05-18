jit-testing
===========

Clang based tool meant to be used for C code unit testing based on LLVM JIT engine

----------
Compiling
==========
This tool assumes is compiled within the clang source tree.

---------
Test File Grammar
========

<test-expr> := <test-file>

<test-file> := [<global-mockup> | <global-setup> | <global-teardown> ] <unit-test-expr>

<global-mockup> := mockup_all "{" <mockup-fixture> "}"

<global-setup> := before_all "{" <test-fixture> "}"

<global-teardown> := after_all "{" <test-fixture> "}"

<unit-test-expr> := <test-definition>+

<test-definition> := [ <test-mockup> | <test-setup>] <test-function>  [ "=" <argument> ] [<test-teardown>]

<test-mockup> := mockup "{" <mockup-fixture> "}"

<test-setup> :=  before "{"  <test-fixture> "}"

<test-function> := <function-call>

<test-teardown> := after "{" <test-fixture> "}"

<test-fixture> := <function-call>*  <var-assignment>*

<mockup-fixture> := <mockup-function>*  <mockup-variable>*

<mockup-function> :=  <function-call> = <argument>  

<mockup-variable> := <var-assignment> 

<var-assignment> := <identifier> = <argument>

<function-call> := <function-name>"(" <function-argument>* ")"

<function-argument> := {<argument> | <buffer-alloc>}

<argument> := [-]<number> | string |  char | <array-initializer>

<number> := int | double |  float

<buffer-alloc> := "[" int [":" int] "]"

<array-initializer> := { <number>+ }

<function-name> := <identifier>

<identifier> := identifier string


Tokens:
	- Function name
	- equal operator
	- int
	- double
	- buffer alloc

keywords:
	before
	afterTest file grammar

<test-expr> := <test-file>

<test-file> := [<global-mockup> | <global-setup> | <global-teardown> ] <unit-test-expr>

<global-mockup> := mockup_all "{" <mockup-fixture> "}"

<global-setup> := before_all "{" <test-fixture> "}"

<global-teardown> := after_all "{" <test-fixture> "}"

<unit-test-expr> := <test-definition>+

<test-definition> := [ <test-mockup> | <test-setup>] <test-function>  [ "=" <argument> ] [<test-teardown>]

<test-mockup> := mockup "{" <mockup-fixture> "}"

<test-setup> :=  before "{"  <test-fixture> "}"

<test-function> := <function-call>

<test-teardown> := after "{" <test-fixture> "}"

<test-fixture> := <function-call>*  <var-assignment>*

<mockup-fixture> := <mockup-function>*  <mockup-variable>*

<mockup-function> :=  <function-call> = <argument>  

<mockup-variable> := <var-assignment> 

<var-assignment> := <identifier> = <argument>

<function-call> := <function-name>"(" <function-argument>* ")"

<function-argument> := {<argument> | <buffer-alloc>}

<argument> := [-]<number> | string |  char | <array-initializer>

<number> := int | double |  float

<buffer-alloc> := "[" int [":" int] "]"

<array-initializer> := { <number>+ }

<function-name> := <identifier>

<identifier> := identifier string


Tokens:
	- Function name
	- equal operator
	- int
	- double
	- buffer alloc

keywords:
	before
	after