[![Build Status](https://travis-ci.org/ComputerScienceHouse/bingehack4.png?branch=master)](https://travis-ci.org/ComputerScienceHouse/bingehack4)

# BingeHack 4 #

## Description ##

The latest iteration of a long-lived line of patches against NetHack by Computer Science House members.
For information about in-game features and changes, please refer to [our GitHub Wiki](https://github.com/ComputerScienceHouse/bingehack4/wiki).

## Development ##

Development occurs on developer forks and they issue pull requests, which the main developers Russ Harmon ([eatnumber1](https://github.com/eatnumber1)) and Chris Lockfort ([clockfort](https://github.com/clockfort)) review in a timely fashion, examining code quality, testing for possible problems, and addressing other miscellaneous issues like game balance.

There are two branches that exist in the main repository; 'master' is for the version of NetHack deployed on [our own server](telnet://games-ng.csh.rit.edu), while 'upstream' exists so that we can retain a symbiotic relationship with upstream NetHack4; fixes for bugs that we have found go upstream from this branch, and fixes they issue themselves also first appear on this branch before being merged in to our own master.

### Requirements ###

* cmake ≥ 2.8.3
* ncurses
* jansson (C-language JSON parsing library) ≥ 2.4
* libpq(xx) (PostgreSQL-project library)
* bison
* flex
* zlib

We use and support Linux and OS X as development environments, as well as both gcc and [clang][clang].
We test our client game terminal support for functionality on:

* OS X/Terminal.app
* Linux/Gnome Terminal
* Windows/Putty
* Windows/MobaXterm

## Useful Links ##

* [Play BingeHack4!](telnet://games-ng.csh.rit.edu)
* [CSH's Travis-CI Build Server: BingeHack4 Project](https://travis-ci.org/ComputerScienceHouse/bingehack4)
* [BingeHack4's Wiki](https://github.com/ComputerScienceHouse/bingehack4/wiki)
* [Upstream NetHack4 Git Repository](http://gitorious.org/nitrohack/ais523/commits/nicehack)

[clang]: http://clang.llvm.org/
