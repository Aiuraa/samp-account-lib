# SA:MP Account Lib

[![sampctl](https://img.shields.io/badge/sampctl-samp--account--lib-2f2f2f.svg?style=for-the-badge)](https://github.com/Aiuraa/samp-account-lib)

A Library that handles user authentication internally without you reinventing the wheel (remaking it again and again) or without you ever touching any complicated mysql thingy again, very easy to use and i think it's newbie friendly.

The autentication uses BCrypt for storing or checking user password, it means user password would be safer to save and hard to crack too!. The password was never actually stored in memory, only when it's used **once** and then the memory will be gone afterwards.

Right this project is Work In Progress or you can call it as Development Phase, meaning it will be **NOT STABLE** to use, since i just throws bunch of ideas in here but never actually tested it since i need to complete the basecode in order to start testing.

## Installation

Simply install to your project:

```bash
sampctl package install Aiuraa/samp-account-lib
```

Include in your code and begin using the library:

```pawn
#include <account-lib>
```

You might want to add some options before using the library, such as:
```pawn
#define ACCLIB_AUTO_FETCH_ACCOUNT
#define ACCLIB_AUTO_KICK_ON_ERROR
#define ACCLIB_ALLOW_MULTI_USER
```

## Usage

<!--
Write your code documentation or examples here. If your library is documented in
the source code, direct users there. If not, list your API and describe it well
in this section. If your library is passive and has no API, simply omit this
section.
-->

For now, you can refer to the [examples](test/examples.pwn) code.

## TODO

Before releasing this project, there is a list that i need to implement before can call it "stable".
Right now the project is in development progress, i've spend my time to write this for myself or maybe for others too!

Feel free to contribute if you can.

- [x] Core component
  - [x] Data fetching
  - [x] Authentication
  - [x] Register
  - [x] Login Sessions
  - [x] Get/Set Account Data

- [ ] Utils 
  - [x] Logging System
  - [ ] Documentation (partial complete)
  - [x] Auto fetch/kick player
  - [x] Example Code

## Testing

<!--
Depending on whether your package is tested via in-game "demo tests" or
y_testing unit-tests, you should indicate to readers what to expect below here.
-->

To test, simply run the package:

```bash
sampctl package run
```
