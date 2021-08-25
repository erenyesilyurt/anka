# Anka Chess Engine
Anka is an artificial intelligence system capable of playing chess. You can use it to play chess against the computer or to analyze your games.

## Requirements
Anka is a command-line only tool. You need to have a UCI compatible chess GUI program to use it comfortably. Some free options are:
- [Arena Chess GUI](http://www.playwitharena.de/)
- [Scid vs. PC](http://scidvspc.sourceforge.net/)

You also need a relatively modern 64-bit CPU that supports instructions like Popcnt.

## Features
- Alpha-beta pruning with principal variation search
- Null move pruning
- Transposition table
- Heuristic evaluation function with material and mobility bonuses, piece square tables, isolated pawn and passed pawn evaluation etc.
- Evaluation parameters tuned with Texel tuning

## Other Notes
Some features available in the UCI protocol, such as pondering, are not supported.

## Thanks to
- Bluefever Software (Vice)
- The Chessprogramming Wiki
- Open source chess engines such as Xiphos, CPWEngine, Rubichess and others...
