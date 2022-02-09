# Anka Chess Engine
Anka is an artificial intelligence system capable of playing chess. You can use it to play chess against the computer or to analyze your games.

## Requirements
Anka is a command-line only tool. You need to have a UCI compatible chess GUI program to use it comfortably. Some free options are:
- [Arena](http://www.playwitharena.de/)
- [Scid vs. PC](http://scidvspc.sourceforge.net/)
- [Tarrasch](https://www.triplehappy.com/)

You also need a relatively modern 64-bit CPU that supports BMI1.

## Features
- Alpha-beta pruning with principal variation search
- Null move pruning, late move reductions, futility pruning
- Transposition table
- Heuristic evaluation function with material and mobility bonuses, piece square tables, isolated pawn and passed pawn evaluation etc.
- Evaluation parameters tuned with Texel tuning
- Syzygy tablebase support thanks to [Pyrrhic](https://github.com/AndyGrant/Pyrrhic/)

## Build Instructions
Although Anka can be built for platforms other than Windows, only the Windows build has been tested. 
1. Download [premake5](https://premake.github.io/download/) for your platform.
2. Generate a Visual Studio solution using premake5. You can use older versions of VS by changing the version number at the end.
```sh
./premake5.exe vs2019 # this creates a Visual Studio 2019 solution
```
3. Open the solution with Visual Studio, set the solution configuration to "Release" and build it as usual.

On Linux, you would generate and use a Makefile instead: (Not tested)
```sh
./premake5 gmake2 # this creates a Makefile
make config=Release
```

For further instructions on using premake5, visit https://premake.github.io/docs/Using-Premake

## Other Notes
Some features available in the UCI protocol (such as pondering, multi-pv and searching selected moves only) are not supported.

Anka implements some custom non-UCI commands for engine testing purposes. All custom commands are
prefixed with anka_.
- anka_print: Print an ASCII board representation along with other position info
- anka_eval: Print static evaluation of the function
- anka_perft d: Run a perft test to depth d with bulk counting at leaf nodes

## Thanks to
- [Bluefever Software](https://www.youtube.com/user/BlueFeverSoft) for their video series on Vice, which introduced me to chess programming
- [The Chessprogramming Wiki](https://www.chessprogramming.org/) and the chess programming community
- The authors of open source chess engines such as [Laser](https://github.com/jeffreyan11/laser-chess-engine), [Xiphos](https://github.com/milostatarevic/xiphos), [CPWEngine](https://github.com/nescitus/cpw-engine), [Rubichess](https://github.com/Matthies/RubiChess) and others
