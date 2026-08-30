# Tools-for-Formal-Logic

## Project Overview
A project focused on developing a SAT solver, alongside a comprehensive set of utilities for parsing, representing, evaluating, and manipulating propositional formulae.

### Syntax used for Propositional Formulae
$$F :== p|~p|(F \lor F)|(F \land F)|(F\rightarrow F)|(F \oplus F)$$

### Functions Implemented
```cpp
  Formula::buildParseTree(std::string formulaString) {} // Given a valid string in proper syntax, builds the formula's parse tree.
  
```

## Installation and Running
```bash
  git clone git@github.com:Bipul027/Tools-for-Formal-Logic.git
  cd Tools-for-Formal-Logic
  make
  ./program
```
