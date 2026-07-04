Help Wanted – Contributors Needed

This project is still under active development and there is a lot of work left to do.

Completing and maintaining a project like this requires a significant amount of time. My biggest limitation is not knowledge or understanding, but free time. I simply cannot implement every feature, fix every issue, and keep up with game updates by myself.

How you can help

Contributions of any size are welcome, including:

- Implementing new features
- Fixing bugs
- Improving existing code
- Refactoring
- Writing documentation
- Testing and reporting issues

Need guidance?

If you'd like to contribute but don't know where to start, just ask me.

I'm happy to explain the project architecture, packet structures, protocol, or guide you through implementing a feature. I'll do my best to help contributors understand the codebase.

Why your contribution matters

Without community contributions, this project will take much longer to complete. With multiple contributors working together, development can move much faster and the project can become more stable and feature-complete.

Every pull request, bug fix, feature, documentation improvement, and suggestion is appreciated.

Thank you for helping make this project better!


# Lords Mobile Bot

A simple C/C++ hybrid prototype bot for Lords Mobile automation.

## ⚠️ Disclaimer

This project is:
- Built solo by a single developer
- A working prototype, not a finished product
- Not fully optimized or structured
- Still under active experimentation

Use at your own risk.

---

## Overview

This is a lightweight automation prototype written in a mix of C and C++.  
It focuses only on core working logic, not advanced features or clean architecture.

The goal is experimentation and learning, not production-level software.

---

## Personal Note

I previously had a better and more advanced version of this bot, but I accidentally lost the source code.

This is my first protocol that I built completely by myself from scratch, and I am still rebuilding and improving it step by step.

---

## Current Status

- ✔ Working prototype
- ✔ Core logic implemented
- ❌ Not fully optimized
- ❌ Codebase not well organized
- ❌ No advanced features yet
- ⚙️ Built and maintained solo

---

## Features

- Basic automation workflow
- Simple task execution system
- Minimal configuration support
- Experimental C/C++ hybrid structure

---

## Input Data (Local Files)

The bot uses local binary files as part of its prototype workflow.

These file paths are currently used in the project:

```c
const char *login1_file = "/sdcard/lmbot/login/MSG_NEWLOGIN_LOGINTOL.bin";
const char *login2_file = "/sdcard/lmbot/login/MSG_NEWLOGIN_LOGINTOP.bin";
```

## Login Files

Extract the lmlogin.zip file

Open gcc.txt for build reference


Build Reference

```shell
aarch64-linux-android-clang -shared -fPIC -o liblmlogin.so \  
lmlogin.c libxhook/xhook.c libxhook/xh_*.c \  
-I./libxhook -llog -ldl  
```

After building shared library you should inject the library in game; and login your account in game; when you login your game account the library will dump session credentials into /sdcard/lmlogin folder!
