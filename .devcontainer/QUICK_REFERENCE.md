# Quick Reference Card

## Build Commands

```bash
# One-command build (recommended)
./build.sh

# Manual build
./autogen.sh
./configure --with-incompatible-bdb
make -j$(nproc)

# Clean build
make distclean
./build.sh

# Install system-wide
sudo make install
```

## Docker Commands

```bash
# Build container image
docker build -t linkcoin-builder .devcontainer/

# Run container
docker run -it --rm \
  -v $(pwd):/workspace \
  -w /workspace/linkcoin-core-v2 \
  linkcoin-builder

# List running containers
docker ps

# Stop container
docker stop <container-id>

# Remove all stopped containers
docker container prune
```

## VS Code Devcontainer

```
F1 → "Dev Containers: Reopen in Container"
F1 → "Dev Containers: Rebuild Container"
F1 → "Dev Containers: Reopen Folder Locally"
```

## Running Linkcoin

```bash
# Start daemon
./src/linkcoind -daemon

# Start with custom datadir
./src/linkcoind -datadir=/path/to/data -daemon

# Start on testnet
./src/linkcoind -testnet -daemon

# Stop daemon
./src/linkcoin-cli stop
```

## Common RPC Commands

```bash
# Get blockchain info
./src/linkcoin-cli getblockchaininfo

# Get wallet info
./src/linkcoin-cli getwalletinfo

# Get block by height
./src/linkcoin-cli getblockhash 400000
./src/linkcoin-cli getblock <hash> 2

# Get development fund info
./src/linkcoin-cli getblocksubsidy 400000

# Get mining template
./src/linkcoin-cli getblocktemplate

# Get peer info
./src/linkcoin-cli getpeerinfo

# Get network info
./src/linkcoin-cli getnetworkinfo
```

## Debugging

```bash
# View debug log
tail -f ~/.linkcoin/debug.log

# View with custom datadir
tail -f /path/to/data/debug.log

# Check if daemon is running
./src/linkcoin-cli getblockcount

# Get help for RPC command
./src/linkcoin-cli help <command>

# List all RPC commands
./src/linkcoin-cli help
```

## ccache Stats

```bash
# View ccache statistics
ccache -s

# Clear ccache
ccache -C

# Set ccache size
ccache -M 5G
```

## File Locations

```bash
# Default datadir (Linux)
~/.linkcoin/

# Config file
~/.linkcoin/linkcoin.conf

# Debug log
~/.linkcoin/debug.log

# Wallet
~/.linkcoin/wallet.dat

# Testnet datadir
~/.linkcoin/testnet3/
```

## Configuration File Example

```ini
# ~/.linkcoin/linkcoin.conf

# Network
listen=1
maxconnections=125

# RPC
server=1
rpcuser=yourusername
rpcpassword=yourpassword
rpcallowip=127.0.0.1

# Mining (if applicable)
mineraddress=<your-linkcoin-address>

# Logging
debug=net
debug=rpc
```

## Useful Flags

```bash
# Reindex blockchain
-reindex

# Rescan wallet
-rescan

# Prune blockchain (keep only 550MB)
-prune=550

# Connect to specific peer
-connect=<ip:port>

# Add node
-addnode=<ip:port>

# Testnet
-testnet

# Regtest (local testing)
-regtest
```

## Development Fund Verification

```bash
# Check if block has dev fund (blocks 365001-3547800)
./src/linkcoin-cli getblock $(./src/linkcoin-cli getblockhash 400000) 2 | grep -A5 "coinbase"

# Get dev fund amount for specific height
./src/linkcoin-cli getblocksubsidy 400000

# Verify dev fund address
# Should be one of:
# - 3P3UvT6vdDJVrbB2mn6WrP8gywpu2Knx8C
# - 34cGTrxRD4VvfbDri6RhQDKPBokfLTNJse
# - 37NpTG2p6gjVeZDmAiPLKNs6Nhj5EfTR55
```

## Troubleshooting Quick Fixes

```bash
# "Database corrupted"
./src/linkcoind -reindex

# "Wallet locked"
./src/linkcoin-cli walletpassphrase <passphrase> <timeout>

# "Cannot connect to peers"
# Check firewall, add nodes manually:
./src/linkcoin-cli addnode <ip:port> add

# "Out of sync"
# Wait for sync, or use -reindex if stuck

# "RPC connection refused"
# Check linkcoin.conf has:
# server=1
# rpcuser=...
# rpcpassword=...
```

## Performance Tips

```bash
# Use ccache for faster rebuilds
export CCACHE_DIR=/workspace/.ccache

# Reduce parallel jobs if low memory
make -j2

# Use tmpfs for faster builds (Linux)
mount -t tmpfs -o size=4G tmpfs /tmp

# Enable debug symbols for debugging
./configure --enable-debug CXXFLAGS="-O0 -g3"
```

## Testing Checklist

- [ ] Build completes without errors
- [ ] `linkcoind -version` shows correct version
- [ ] Daemon starts successfully
- [ ] Syncs with network
- [ ] RPC commands work
- [ ] Development fund validated (blocks 365001-3547800)
- [ ] Compatible with old linkcoin-core nodes
- [ ] Wallet operations work
- [ ] Can create and broadcast transactions

