# Lambda

A basic lambda calculus compiler.

## Setup

This project uses [bear](https://github.com/rizsotto/bear) to maintain dependencies.

```bash
git clone https://github.com/hteng2/lambda-compiler.git
bear -- make
```

## Overview

The only object in this language is the expression. An expression takes many forms:

1. An integer: `1234`
2. A "unit": `~`
3. An operation on one or two expressions: `-1`, `1 + 2 * 3`
4. A lambda expression: `\x.x+1`
5. An application: `(\x.x+1) 2`

To resolve types and recursion, there is the self-referential lambda expression:

```
(\x : s . (x=0) 1 (s (x-1)))
```
