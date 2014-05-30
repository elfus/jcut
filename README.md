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

<test-file> := [<global-mockup> | <global-setup> | <global-teardown> ] <unit-tests>

<global-mockup> := mockup_all "{" <mockup-fixture> "}"

<global-setup> := before_all "{" <test-fixture> "}"

<global-teardown> := after_all "{" <test-fixture> "}"

<unit-tests> := <test-group>

<test-group> := <test-definition>+ | group [<identifier>] "{" <test-group>+ "}"

<test-definition> := [<test-info> | <test-mockup> | <test-setup>] <test-function> [<test-teardown>]

<test-info> := test <identifier>

<test-mockup> := mockup "{" <mockup-fixture> "}"

<test-setup> :=  before "{"  <test-fixture> "}"

<test-function> := <function-call> [<expected-result>]


<test-teardown> := after "{" <test-fixture> "}"

<test-fixture> := <function-call>*  <var-assignment>* <expected-expr>*

<mockup-fixture> := <mockup-function>*  <mockup-variable>*

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
