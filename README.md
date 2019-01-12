# Promise++
Promise++ is a C++ library that integrates [Promise A+](https://promisesaplus.com/#notes) specification using C++11 features.

## Installation & Setup
1. Download or clone the [Promise++](https://github.com/SEDS/promisepp) repository.
2. Include **Promise.h** in the C++ file you want to use promises.
    - See a sample version in source.cpp

## Easy Makefile Creation
1. Install [Makefile Project Creator (MPC)](https://github.com/DOCGroup/MPC) on your system.
    - We use MPC to easily create Makefiles and other code environments such as visual studio projects.
    - you'll need [Perl](https://www.perl.org/get.html) on your system to use MPC.
3. In a terminal/command-line, navigate to the Promise++ repo and run `perl mwc.pl -type make .`
    - This will create the Makefile for executing our sample code found in source.cpp.
4. To compile run `make`
5. To execute run `./Promises`
