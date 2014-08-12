/*
 * File:   TestGeneratorVisitor.h
 * Author: aortegag
 *
 * Created on May 1, 2014, 7:39 PM
 */

#ifndef TESTGENERATORVISITOR_H
#define	TESTGENERATORVISITOR_H

#include "Visitor.h"
#include <string>
#include <vector>
#include <tuple>
#include <map>

namespace llvm {
    class Function;
    class Module;
    class BasicBlock;
}

#include "llvm/IR/IRBuilder.h"
#include "TestParser.h"

using namespace tp;

class TestGeneratorVisitor : public Visitor {
private:
    llvm::Module *mModule;
    llvm::IRBuilder<> mBuilder;
    /// Used as a temp variable to the instruction that calls the function to be tested.
    llvm::CallInst *mTestFunctionCall;
    /// Every time we visit a TestDefinitionExpr a new BasicBlock is created.
    llvm::BasicBlock *mCurrentBB;
    /// Instructions generated for each TestDefinitionExpr
    std::vector<llvm::Instruction*> mInstructions;
    /// This vector is used to hold all the instructions needed to allocate memory
    /// for pointers. Since we have different functions for the setup, function
    /// under test, teardown and cleanup, the previous implementation allocated
    /// the memory and then passed a pointer to the FUD on the same llvm Function,
    /// however the allocation was being done in a different function on the stack
    /// and after exiting the setup function that memory was no longer valid. Thus
    /// we use this vector to store those instructions when they generated, but
    /// we actually use the instructions when we really need them, that is when we
    /// VisitTestFunction we just insert these instructions at the very beginning
    /// of the mInstructions vector.
    std::vector<llvm::Instruction*> mPtrAllocation;
    /// Used to hold the arguments for each FunctionCallExpr
    std::vector<llvm::Value*> mArgs;
    /// Used to hold backup values of global variables and their original values
    /// for each of the tests
    /// @todo Add support for structures
    std::vector<tuple<llvm::Value*,llvm::GlobalVariable*>> mBackup;
    /// Used to hold backup values of global variables and their original values
    /// for groups
    /// first template argument is the dst (backup), second  is the src
    /// (original) variable
    std::vector<tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>> mBackupTemp;
    std::vector<tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>> mBackupTest;
    std::vector<tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>> mBackupGroup;
    llvm::Value *mReturnValue;
    std::map<string,bool> mMockupNames;//used to create unique mockup names
    /// Store all the warnings for a single TestDefinition.
    std::vector<Exception> mWarnings;
    std::string mCurrentFud; // Current Function Under Test

    /**
 * Creates a new Value of the same Type as type with real_value
 */
    llvm::Value* createValue(llvm::Type* type,
                             const std::string& real_value);

    /**
     * Takes existing instructions in vector mInstructions to generate a function
     * of the given name.
     *
     * If use_mReturnValue is true, the function created will return the value
     * pointed to by the attribute mReturnValue, if false it will always return 0.
     *
     * If restore_backup is true the function will add code to restore the
     * original values of global variables stored in the mBackup vector attribute.
     *
     * After returning, this function will have cleared the vector of instructions,
     * the vector of GlobalVariables to be backed up, and clear the IRBuilder
     * insertion point.
     *
     * @param name Name of the function
     * @param use_mReturnValue
     * @param restore_backup
     * @return Function
     */
    llvm::Function* generateFunction(const string& name, bool use_mReturnValue = false);

    /**
     * Saves all the global variables loaded in mBackup in a new global variable.
	 *
     * This method saves the global variables backup in an array of 2-tuples
     * containing the original global variable and the newly create global
     * variable backup. The array is the attribute mBackupGroup. This method
     * is used to save the global variable state for each group.
     *
     * This method needs to be called before creating a function with the
     * generateFunction method.
     *
     * @note This method needs to be called when we VisitGlobalSetup, because
     * that's when all the code has been generated for the global variable
     * assignment. However, the order in which we VisitGlobalSetup is descending
     * order of group before we VisitGlobaTeardown. In order to differentiate
     * the GlobalVariables backed up for each group we are inserting a 2-tuple
     * of nullptrs to mark the beginning of a new group.
     *
     */
    void saveGlobalVariables();

    /**
     * Restores the backed up variables with the saveGlobalVariables() method.
     *
     * @note This method needs to be called when we VisitGlobalTeardown, because
     * that's when all the code has been generated for the teardown of a group.
     * This method needs to take into consideration the fact that the teardown
     * statements are visited in ascending order from the innermost group to the
     * outermost group, this is due to the way GlobalSetup and GlobalTeardown
     * are visited in method TestGroup::accept(). Therefore we use the vector
     * mBackupGroup as a stack in order to pop out the global variables to be
     * restored and taking into a account the marker of 2-tuple with nullptrs.
     */
    void restoreGlobalVariables(std::vector<tuple<llvm::GlobalVariable*,llvm::GlobalVariable*>>&);

    /**
     * Extracts the initializer values in a recursive fashion and stores them
     * in the global_struct.
     *
     * @note This method only support Initialization Lists, designated initializers
     * are NOT supported.
     *
     * @param global_struct The global structure we are pointing to.
     * @param init The values we want to extract
     * @param ndxs Indices used by the GEP instruction to store the correct values.
     */
    void extractInitializerValues(llvm::Value* global_struct,
                                  const StructInitializer* init,
                                  vector<llvm::Value*>* ndxs);

    llvm::Value* createIntComparison(ComparisonOperator::Type,llvm::Value* LHS, llvm::Value* RHS);
    llvm::Value* createFloatComparison(ComparisonOperator::Type,llvm::Value* LHS, llvm::Value* RHS);

    /**
     * Allocates and initializes a buffer of type ptrType->elementType using BufferAlloc information.
     * The resulting instructions generated will be stored in the vector 'instructions'.
     *
     * @param[in] ptrType
     * @param[in] ba
     * @param[out] instructions A vector of instructions to store the code we generate.
     * @return
     */
    llvm::AllocaInst* bufferAllocInitialization(llvm::Type* ptrType, tp::BufferAlloc *ba,
                                                std::vector<llvm::Instruction*>& instructions);

    string getUniqueTestName(const string& name);
public:
    TestGeneratorVisitor(llvm::Module *mod);
    TestGeneratorVisitor(const TestGeneratorVisitor&) = delete;
    ~TestGeneratorVisitor() {}

    void VisitFunctionArgument(FunctionArgument *);
    void VisitFunctionCall(FunctionCall *);
    // Used to compare the results from the function under test against an expected result.
    void VisitExpectedResult(ExpectedResult *);
    // Used to compare expressions in 'after or after_all' statements.
    void VisitExpectedExpression(ExpectedExpression *);
    void VisitMockupFunction(MockupFunction*);
    void VisitVariableAssignment(VariableAssignment *);
    /// Generates an LLVM Function that calls a function, assigns a variable or
    /// checks an expected expression.
    void VisitTestSetup(TestSetup *);
    /// Generates an LLVM Function that calls a function, assigns a variable or
    /// checks an expected expression.
    void VisitTestTeardown(TestTeardown *);
    /// Generates the LLVM Function that calls our function under test (FUD)
    void VisitTestFunction(TestFunction *);
    /// Restores whatever global variable was modified in VisitTestSetup or VisitTestTeardown
    void VisitTestDefinition(TestDefinition *);
    void VisitTestDefinitionFirst(TestDefinition *);
    /// Generates an LLVM Function that maps to before_all
    void VisitGlobalSetup(GlobalSetup *);
    /// Generates an LLVM Function that maps to after_all
    void VisitGlobalTeardown(GlobalTeardown *);
    /// Generates an LLVM Function that restores whatever was done in before_all
    /// and after_all
    void VisitTestGroup(TestGroup *);
    void VisitTestGroupFirst(TestGroup *);

};

#endif	/* TESTGENERATORVISITOR_H */

