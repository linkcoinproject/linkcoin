Linkcoin Core integration/staging tree
=====================================

https://linkcoin.org

Copyright (c) 2009-2014 Bitcoin Developers
Copyright (c) 2014-2025 Linkcoin Developers

What is Linkcoin?
----------------

Linkcoin is a Litecoin-based cryptocurrency using scrypt as proof-of-work.
 - 4 minute block targets
 - 80 LNC subsidy, halving every 400,000 blocks (~4 years)
 - ~84 million total coins
 - Scrypt-1024-1-1-256 PoW algorithm
 - Elastic Exponential Difficulty (EED) — dynamic retarget every block
 - Addresses start with `L`

For more information, see https://linkcoin.org.

Building
--------

Linkcoin Core v2.0.0 uses the modern autotools build system.

### Dependencies

```bash
# Build dependencies (one-time)
cd depends
make HOST=x86_64-pc-linux-gnu -j$(nproc)
cd ..
```

### Build Daemon

```bash
./autogen.sh
./configure --without-gui --disable-wallet
make -j$(nproc)
```

Binary: `src/linkcoind`

### Build with Qt GUI

```bash
./autogen.sh
./configure
make -j$(nproc)
```

Binary: `src/qt/linkcoin-qt`

### Windows Cross-Compile

```bash
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
cd ..
./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure
make -j$(nproc)
```

Network Parameters
------------------

| Parameter | Mainnet | Testnet |
|---|---|---|
| P2P Port | 7200 | 72555 |
| RPC Port | 9600 | 96555 |
| Magic Bytes | fbc0b6db | fcc1b7dc |
| Address Prefix | 48 (L...) | 111 (m/n...) |
| P2SH Prefix | 5 (3...) | 196 (2...) |
| WIF Prefix | 176 (Q...) | 239 (c...) |

Consensus
---------

- **Genesis**: `2865bdde500f4c65eb97ac69c9bc29850a0571a19f2121613c283fbd2d334bd7`
- **Block time**: 240 seconds (4 minutes)
- **Difficulty**: Elastic Exponential Difficulty (EED) retargets every block
  - Stage 0 (0-387): Original retarget
  - Stage 1 (388-2499): DigiShield
  - Stage 2 (2500-4999): Classic2 Adaptive
  - Stage 3 (5000-5299): EED Buggy
  - Stage 4 (5300-5606): EED V1
  - Stage 5 (5607-74999): EED V2 (ASERT anchor-based)
  - Stage 6 (75000+): EED V3 (per-block ASERT)
- **Subsidy**: 80 LNC + halving every 400k blocks, IPO block at height 10 (500k LNC)
- **MAX_MONEY**: 84,000,000 LNC
- **COINBASE_MATURITY**: 100 blocks

Future Features (disabled, ready for activation)
------------------------------------------------

- **AuxPoW** (chain ID 0x4C4E) — merged mining support
- **SegWit** — BIP141/143 witness program
- **Taproot** — BIP340-342 Schnorr/Taproot
- **MWEB** — MimbleWimble Extension Blocks

All are pre-configured in chainparams. Activation requires setting the height.

License
-------

Linkcoin Core is released under the MIT license. See `COPYING`.

Development process
-------------------

The `master` branch is the stable release. The `rebase-ltc` branch is the
current development branch (Litecoin 0.21 base with Linkcoin consensus).

Pull requests are welcome. For major changes, start a discussion first.

Testing
-------

```bash
# Unit tests
make check

# Functional tests
test/functional/test_runner.py
```
