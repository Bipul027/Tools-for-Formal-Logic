# Tools for Formal Logic

Tools for Formal Logic is a C++17 implementation of a small propositional-logic processing framework. The repository provides data structures and algorithms for parsing propositional formulae into syntax trees, evaluating formulae under an assignment, eliminating selected logical connectives, converting formulae to negation normal form and conjunctive normal form, and checking validity through the resulting CNF representation.

The project is structured with a future SAT-solving workflow in mind. In its current form, however, the executable does not implement a general satisfiability search procedure; its implemented decision procedure is a CNF-based validity check.

## 1. Scope and Objectives

The project provides the following core capabilities:

1. Representation of propositional formulae as binary/unary syntax trees.
2. Parsing of formula strings into syntax trees.
3. Reconstruction of formula strings from syntax trees.
4. Evaluation of formulae under a Boolean assignment.
5. Collection of propositional symbols appearing in a formula.
6. Elimination of implication and exclusive-or connectives.
7. Conversion to negation normal form (NNF).
8. Conversion from NNF to conjunctive normal form (CNF).
9. Representation and manipulation of CNF formulae as sets of clauses and literals.
10. Validity checking of CNF formulae and complete formulae through CNF transformation.

The implementation is intended primarily as a compact software foundation for experimenting with algorithms from propositional logic and automated reasoning.

## 2. Propositional Formula Syntax

Formulae are written using a fully parenthesized syntax for binary operators.

The logical language implemented by the parser uses the following operators:

| Symbol | Meaning      |       Arity |        |
| ------ | ------------ | ----------: | ------ |
| `~`    | Negation     |       Unary |        |
| `&`    | Conjunction  |      Binary |        |
| `      | `            | Disjunction | Binary |
| `>`    | Implication  |      Binary |        |
| `^`    | Exclusive-or |      Binary |        |

The intended recursive form is

```text
F ::= p
    | ~F
    | (F&F)
    | (F|F)
    | (F>F)
    | (F^F)
```

where `p` denotes a propositional symbol.

Examples of formulae accepted by the parser include:

```text
p
~p
(p&q)
(p|q)
(p>q)
(p^q)
~(p&q)
((p&q)|(~r>s))
```

Binary formulae should be parenthesized explicitly. The executable removes whitespace from the input before constructing the formula, so the following expressions are treated equivalently by the command-line program:

```text
(p & q)
(p&q)
```

### 2.1 Semantics

The operators have their standard Boolean semantics:

```text
~P       = not P
P & Q    = P and Q
P | Q    = P or Q
P ^ Q    = P xor Q
P > Q    = (not P) or Q
```

No Boolean constants such as `true`, `false`, `T`, `F`, `⊤`, or `⊥` are defined by the current parser as dedicated constants.

## 3. System Architecture

The implementation is divided into four logical components:

```text
Tools-for-Formal-Logic/
├── wff/
│   ├── validator.h
│   ├── validator.cpp
│   └── tree_to_str.cpp
├── evaluator/
│   ├── evaluator.h
│   └── evaluator.cpp
├── cnf/
│   ├── cnf.h
│   ├── cnf.cpp
│   ├── converting_functions.h
│   └── converting_functions.cpp
├── validity/
│   ├── validity.h
│   └── validity.cpp
├── main.cpp
├── Makefile
├── .gitignore
└── README.md
```

The dependencies between the major components can be summarized as follows:

```text
                 ┌──────────────────┐
                 │    Formula/Node  │
                 │      (wff)       │
                 └────────┬─────────┘
                          │
              ┌───────────┼────────────┐
              │           │            │
              ▼           ▼            ▼
        ┌──────────┐ ┌──────────┐ ┌──────────────┐
        │ Evaluator│ │ Stringify│ │ Transformations│
        │(evaluator)│ │ (wff)   │ │    (cnf)     │
        └──────────┘ └──────────┘ └───────┬──────┘
                                         │
                                         ▼
                                  ┌────────────┐
                                  │    CNF     │
                                  │Representation│
                                  └─────┬──────┘
                                        │
                                        ▼
                                  ┌────────────┐
                                  │  Validity  │
                                  └────────────┘
```

## 4. Formula Representation

### 4.1 `Node`

`Node` is the fundamental syntax-tree element declared in `wff/validator.h`.

Each node contains:

```cpp
std::string nodeString;
Node *left;
Node *right;
```

For an operator node, `nodeString` contains one of `~`, `&`, `|`, `^`, or `>` and its children represent the operands.

For an atomic proposition, `nodeString` contains the proposition symbol and both child pointers are null.

Unary negation is represented using the left child:

```text
    ~
    |
    p
```

A binary connective is represented using both children:

```text
      &
     / \
    p   q
```

### 4.2 `Formula`

`Formula` is the public abstraction used to construct and manipulate formulae.

Its main interface is:

```cpp
class Formula {
public:
    Node *root;

    Formula();
    Formula(Node *root);
    Formula(std::string s);

    Node *buildParseTree(std::string &formulaString);
    void printTree();
    bool evaluateFormula(assignment &assign, Node *root);
};
```

The string constructor immediately parses its argument and stores the resulting tree in `root`.

## 5. Parsing

The parser is implemented by `Formula::buildParseTree` in `wff/validator.cpp`.

The parsing procedure is recursive.

### 5.1 Negation

When the first character is `~`, the parser removes the leading `~`, recursively parses the remaining expression, and creates a unary node:

```text
~F
```

becomes

```text
    ~
    |
    F
```

### 5.2 Binary Formulae

For a parenthesized expression, the parser scans the expression while maintaining a parenthesis-depth counter. A connective is selected only when it occurs at the current top level of that expression.

For example:

```text
((p&q)|r)
```

has `|` as the top-level operator, whereas `&` belongs to the nested left operand.

The resulting tree is therefore equivalent to:

```text
       |
      / \
     &   r
    / \
   p   q
```

### 5.3 Tree Validation

`Node::isNodeValid()` checks local structural conditions, such as whether operator nodes have the expected children and whether atomic nodes are leaves. The current parsing path does not invoke this method automatically as a separate validation phase. Consequently, callers should provide formulae conforming to the supported syntax.

## 6. Formula Evaluation

Evaluation is implemented by `Formula::evaluateFormula`.

The evaluator recursively computes the Boolean value of each subtree.

For an atomic node, the evaluator looks up its value in an `assignment` object. For operators, it applies the corresponding Boolean operation recursively.

The supported semantics are:

```text
~F       -> !F
F&G      -> F && G
F|G      -> F || G
F^G      -> F xor G
F>G      -> (!F) || G
```

## 7. Assignments and Models

The `assignment` structure is defined in `evaluator/evaluator.h`:

```cpp
struct assignment {
    std::map<std::string, bool> model;

    void buildModel(Node *root);
    void printModel();
};
```

`buildModel` traverses the syntax tree and inserts every atomic proposition into the map.

The current implementation assigns `true` to each discovered proposition. Thus, the model constructed by `buildModel` is the all-true assignment.

For a formula containing `p`, `q`, and `r`, the generated model is conceptually:

```text
p : 1
q : 1
r : 1
```

The current command-line program therefore evaluates each input formula under the all-true assignment. It does not provide a command-line interface for entering an arbitrary truth assignment.

## 8. Formula Reconstruction

`treeToStr` reconstructs a formula string from a syntax tree.

Atomic nodes are returned directly. Negation is emitted as a prefix operator, and binary operators are reconstructed using explicit parentheses.

For example, the tree representing

```text
(p&(q|r))
```

is reconstructed as:

```text
(p&(q|r))
```

This function is useful for observing the result of transformation passes.

## 9. Logical Transformations

The conversion functions are implemented in `cnf/converting_functions.cpp`.

The normalization pipeline used by the project is:

```text
Formula
   │
   ▼
IMPL_FREE
   │
   ▼
XOR_FREE
   │
   ▼
NNF
   │
   ▼
convertToCNF
   │
   ▼
CNF
```

`UNI_ONLY` combines implication elimination and XOR elimination.

### 9.1 Implication Elimination

`IMPL_FREE` replaces

```text
F > G
```

with the logically equivalent formula

```text
(~F)|G
```

The transformation is applied recursively throughout the tree.

### 9.2 Exclusive-Or Elimination

`XOR_FREE` replaces

```text
F ^ G
```

with

```text
(F&~G)|(~F&G)
```

which is the standard definition of exclusive-or using conjunction, disjunction, and negation.

The implementation makes copies of the affected subtrees so that both occurrences required by the expansion are represented correctly.

### 9.3 Combined Unary-Operator Elimination

`UNI_ONLY` performs both transformations:

```cpp
Node *UNI_ONLY(Node *root);
```

Conceptually:

```text
UNI_ONLY(F) = XOR_FREE(IMPL_FREE(F))
```

After this transformation, the formula contains only:

```text
~   &   |
```

among the logical operators.

### 9.4 Negation Normal Form

`NNF` assumes that implication and XOR have already been removed.

It applies the standard equivalences:

```text
~~F       -> F
~(F&G)    -> (~F)|(~G)
~(F|G)    -> (~F)&(~G)
```

After conversion to NNF, negation occurs only directly above atomic propositions.

For example:

```text
~((p&q)|r)
```

is converted to

```text
(~p|~q)&~r
```

### 9.5 Conjunctive Normal Form

`convertToCNF` assumes that its input is already in NNF.

A CNF formula has the form

```text
C1 & C2 & ... & Cn
```

where each clause is a disjunction of literals:

```text
(l1 | l2 | ... | lk)
```

The implementation represents a CNF directly as a set of clauses rather than as another `Node` tree.

Conjunction is implemented by merging clause sets.

Disjunction is implemented by the distributive law. The `DISTR` function computes a CNF for the disjunction of two CNF formulae.

For example:

```text
(p&q)|r
```

is transformed into:

```text
(p|r)&(q|r)
```

## 10. CNF Representation

The `CNF` class is defined in `cnf/cnf.h`.

Its internal representation is:

```cpp
std::set<
    std::set<
        std::pair<std::string, bool>
    >
> clauses;
```

The levels have the following interpretation:

```text
CNF
└── set of clauses
    └── set of literals
        └── (proposition, polarity)
```

The Boolean field of a literal records polarity:

```text
true   -> positive literal
false  -> negated literal
```

Therefore:

```text
(p | ~q | r)
```

is represented conceptually by the set

```text
{
    {("p", true), ("q", false), ("r", true)}
}
```

### 10.1 `CNF` Interface

The principal operations are:

```cpp
CNF();
CNF(std::set<std::pair<std::string, bool>> &clause);
CNF(std::set<std::set<std::pair<std::string, bool>>> &clauses);
CNF(std::string &prop, bool state);

bool empty();
int size();
void print();
void merge(CNF &other);
std::set<std::pair<std::string, bool>> pop();
std::set<std::set<std::pair<std::string, bool>>> CNFtoTree();
```

`merge` inserts all clauses from another CNF into the current CNF.

`pop` removes and returns one clause.

`CNFtoTree` returns the underlying clause-set representation used by the validity checker.

## 11. Validity Checking

Validity checking is implemented in `validity/validity.cpp`.

The public interface is:

```cpp
bool isCNFValid(CNF Formula);
bool isFormulaValid(std::string FormulaString);
```

### 11.1 CNF Validity Criterion

For a CNF formula

```text
C1 & C2 & ... & Cn
```

to be a tautology, every clause `Ci` must itself be a tautological disjunction.

A clause is tautological when it contains both a proposition and its negation. For example:

```text
(p|~p|q)
```

is always true.

The implementation checks every literal `(p, polarity)` in a clause for the complementary literal `(p, !polarity)`.

If every clause contains such a complementary pair, `isCNFValid` returns `true`.

### 11.2 Complete Formula Validity

`isFormulaValid` applies the full normalization chain:

```text
input string
    ↓
parse tree
    ↓
IMPL_FREE
    ↓
XOR_FREE
    ↓
NNF
    ↓
CNF
    ↓
clause-wise tautology test
```

The result is a Boolean indication of whether the input formula is valid under classical propositional semantics, subject to the parser and conversion assumptions described in this document.

## 12. Command-Line Program

The executable entry point is `main.cpp`.

The current program performs the following sequence for one input line:

1. Read a formula from standard input.
2. Remove all spaces.
3. Construct a `Formula` and its parse tree.
4. Print the parse tree using an in-order traversal.
5. Construct the all-true assignment for all atomic symbols.
6. Print the assignment.
7. Evaluate the formula under that assignment.
8. Print the canonical string reconstructed from the tree.
9. Eliminate implication and XOR.
10. Print the resulting formula.
11. Convert the formula to NNF and print it.
12. Convert the NNF formula to CNF.
13. Print the CNF.
14. Check CNF validity and print the result.

## 13. Building the Project

### 13.1 Requirements

The project requires:

* A C++ compiler supporting C++17.
* GNU Make.

The supplied Makefile uses `g++` and the following compiler settings:

```text
-Wall -Wextra -g -std=c++17
```

### 13.2 Clone

```bash
git clone https://github.com/Bipul027/Tools-for-Formal-Logic.git
cd Tools-for-Formal-Logic
```

SSH may also be used:

```bash
git clone git@github.com:Bipul027/Tools-for-Formal-Logic.git
```

### 13.3 Build

```bash
make
```

The Makefile compiles the implementation files into object files and links them into an executable named `program`.

### 13.4 Clean

```bash
make clean
```

This removes generated object files and the `program` executable.

## 14. Running the Program

The executable reads exactly one formula from standard input.

```bash
./program
```

Then provide a formula, for example:

```text
((p&q)|r)
```

Alternatively:

```bash
echo '((p&q)|r)' | ./program
```

The output contains the intermediate representations produced by the pipeline, including the assignment, evaluation result, reconstructed formula, implication/XOR-free formula, NNF, CNF, and validity result.

## 15. Public Interfaces by Module

### `wff`

Provides the base representation and parsing functionality.

| Symbol                       | Purpose                                          |
| ---------------------------- | ------------------------------------------------ |
| `Node`                       | Syntax-tree node                                 |
| `Node::isNodeValid()`        | Local structural validation                      |
| `Formula`                    | Formula abstraction                              |
| `Formula::buildParseTree()`  | Parse a formula string                           |
| `Formula::printTree()`       | Print the parse tree traversal                   |
| `Formula::evaluateFormula()` | Evaluate a formula under a model                 |
| `treeToStr()`                | Serialize a syntax tree back to a formula string |

### `evaluator`

Provides Boolean assignments.

| Symbol         | Purpose                                      |
| -------------- | -------------------------------------------- |
| `assignment`   | Maps propositional symbols to Boolean values |
| `buildModel()` | Extract symbols and assign `true`            |
| `printModel()` | Print the current assignment                 |

### `cnf`

Provides CNF data structures and formula transformations.

| Symbol           | Purpose                                             |
| ---------------- | --------------------------------------------------- |
| `CNF`            | Clause/literal representation                       |
| `IMPL_FREE()`    | Eliminate implication                               |
| `XOR_FREE()`     | Eliminate XOR                                       |
| `UNI_ONLY()`     | Apply both unary-normalization passes               |
| `NNF()`          | Convert implication/XOR-free formulae to NNF        |
| `DISTR()`        | Distribute disjunction over conjunction in CNF form |
| `merge()`        | Combine CNF clause sets                             |
| `convertToCNF()` | Convert NNF to CNF                                  |

### `validity`

Provides validity predicates.

| Symbol             | Purpose                                        |
| ------------------ | ---------------------------------------------- |
| `isCNFValid()`     | Check validity of a CNF representation         |
| `isFormulaValid()` | Parse, normalize, convert, and check a formula |

## 16. Repository Design

The repository separates concerns according to the stages of formal processing:

* `wff/` contains the formula representation and parser.
* `evaluator/` contains model construction and model display.
* `cnf/` contains normalization algorithms and CNF representation.
* `validity/` contains logical validity predicates.
* `main.cpp` serves as a demonstration executable tying the modules together.

This organization makes it possible to extend individual stages without placing parsing, transformation, and decision procedures in a single implementation unit.
