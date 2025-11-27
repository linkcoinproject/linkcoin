Linkcoin integration/staging tree
================================

http://www.linkcoin.org

Copyright (c) 2009-2014 Bitcoin Developers
Copyright (c) 2011-2014 Linkcoin Developers

What is Linkcoin?
----------------

Linkcoin is a LiteCoin Clone, a lite version of Bitcoin using scrypt as a proof-of-work algorithm.
 - 4 minute block targets
 - subsidy halves in 400k blocks (~4 years)
 - ~84 million total coins

The rest is the same as Bitcoin.
 - 80 coins per block
 - retarget difficulty every block

For more information, as well as an immediately useable, binary version of
the Linkcoin client sofware, see http://www.linkcoin.org.

Building
--------

### Compiler Machine
**Recommended Environment:** Debian 7 (Wheezy)
This environment is required for binary compatibility with older systems and to ensure the `depends` system works as expected with the provided Qt 4.8.7 patches.

### Static Build with Depends System (Recommended)

Linkcoin uses the Bitcoin `depends` system to build fully static binaries with no external dependencies. This is the only supported way to build the release binaries.

**Features:**
- ✅ Fully static binaries (portable, no dependencies)
- ✅ Cross-platform (Linux, Windows)
- ✅ Reproducible builds
- ✅ Qt 4.8.7 (compatible with old MinGW 4.6+)

#### 1. Build Dependencies
First, build the dependencies for your target platform. This step only needs to be done once or when dependencies change.

**For Linux (x86_64):**
```bash
cd depends
make HOST=x86_64-pc-linux-gnu -j$(nproc)
cd ..
```

**For Windows (x86_64):**
```bash
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
cd ..
```

#### 2. Build Linkcoin
Use the provided `build.sh` script to compile the daemon and GUI.

**Linux:**
```bash
./build.sh --linux
```
*Result:* `src/linkcoind` and `linkcoin-qt`

**Windows:**
```bash
./build.sh --windows
```
*Result:* `src/linkcoind.exe` and `release/linkcoin-qt.exe`

**Build Options:**
- `--daemon-only`: Build only `linkcoind` (skip Qt GUI)
- `--help`: Show all available options

---

### Manual Build (Legacy/Development)

For development purposes, you can still use the legacy makefiles, but this is not recommended for release builds.

**Linux Daemon:**
```bash
cd src
make -f makefile.unix
```

**Linux GUI:**
```bash
qmake
make
```


For development with system libraries:

```bash
# Build daemon
cd src
make -f makefile.unix

# Build GUI
qmake
make
```

**Note:** Produces dynamically-linked binaries requiring system libraries.

License
-------

Linkcoin is released under the terms of the MIT license. See `COPYING` for more
information or see http://opensource.org/licenses/MIT.

Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the Linkcoin
development team members simply pulls it.

If it is a *more complicated or potentially controversial* change, then the patch
submitter will be asked to start a discussion with the devs and community.

The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see `doc/coding.txt`) or are
controversial.

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/linkcoin-project/linkcoin/tags) are created
regularly to indicate new official, stable release versions of Linkcoin.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test. Please be patient and help out, and
remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write unit tests for new code, and to
submit new unit tests for old code.

Unit tests for the core code are in `src/test/`. To compile and run them:

    cd src; make -f makefile.unix test

Unit tests for the GUI code are in `src/qt/test/`. To compile and run them:

    qmake BITCOIN_QT_TEST=1 -o Makefile.test bitcoin-qt.pro
    make -f Makefile.test
    ./linkcoin-qt_test

