#!/usr/bin/env python3
# Copyright (c) 2024 The Linkcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test AuxPoW activation, block version bits, and AuxPoW block submission."""

import struct
import time
from io import BytesIO

from test_framework.messages import (
    CBlock,
    CBlockHeader,
    CTransaction,
    CTxIn,
    CTxOut,
    COutPoint,
    FromHex,
    ToHex,
    hash256,
    ser_uint256,
    deser_uint256,
    ser_compact_size,
    deser_compact_size,
    uint256_from_str,
    uint256_from_compact,
    sha256,
    COIN,
)
from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment,
    script_BIP34_coinbase_height,
)
from test_framework.script import CScript, OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    hex_str_to_bytes,
)

def bytes_to_hex_str(b):
    return b.hex()

# Linkcoin AuxPoW constants
VERSION_AUXPOW = (1 << 8)       # 0x100
VERSION_CHAIN_START = (1 << 16)  # 0x10000
VERSIONBITS_TOP_BITS = 0x20000000
LNC_CHAIN_ID = 0x4C4E  # consensus param
# The chain ID in the block version includes VERSIONBITS_TOP_BITS after SetChainId
# SetChainId(0x4C4E) -> 0x4C4E0004, then |= VERSIONBITS_TOP_BITS -> 0x6C4E0004
# GetChainId() = nVersion >> 16 = 0x6C4E
LNC_CHAIN_ID_BLOCK = (VERSIONBITS_TOP_BITS >> 16) | LNC_CHAIN_ID  # 0x6C4E
MERGED_MINING_HEADER = b'\xfa\xbe' + b'mm'

# Regtest activation heights
BIP34_HEIGHT = 0
BIP65_HEIGHT = 100
BIP66_HEIGHT = 100
CSV_HEIGHT = 200
SEGWIT_HEIGHT = 300
TAPROOT_HEIGHT = 400
MWEB_HEIGHT = 500
AUXPOW_HEIGHT = 600


def get_chain_id(version):
    """Extract chain ID from block version."""
    return (version >> 16) & 0xFFFF


def get_base_version(version):
    """Extract base version from block version."""
    return version % VERSION_AUXPOW


def is_auxpow(version):
    """Check if VERSION_AUXPOW flag is set."""
    return (version & VERSION_AUXPOW) != 0


def is_legacy(version):
    """Check if block is legacy (pre-AuxPoW format)."""
    return version == 1 or (version == 2 and get_chain_id(version) == 0)


def get_expected_index(nonce, chain_id, height):
    """Compute expected index in chain merkle tree (same as C++ getExpectedIndex)."""
    rand = nonce
    rand = (rand * 1103515245 + 12345) & 0xFFFFFFFF
    rand = (rand + chain_id) & 0xFFFFFFFF
    rand = (rand * 1103515245 + 12345) & 0xFFFFFFFF
    return rand % (1 << height)


def check_merkle_branch(hash_val, branch, index):
    """Compute merkle root from leaf and branch (same as CAuxPow::CheckMerkleBranch)."""
    h = hash_val
    for sibling in branch:
        if index & 1:
            h = uint256_from_str(hash256(ser_uint256(sibling) + ser_uint256(h)))
        else:
            h = uint256_from_str(hash256(ser_uint256(h) + ser_uint256(sibling)))
        index >>= 1
    return h


def compute_merkle_branch(leaves, index):
    """Compute merkle branch for a leaf at the given index."""
    branch = []
    level = list(leaves)
    while len(level) > 1:
        new_level = []
        for i in range(0, len(level), 2):
            left = level[i]
            right = level[i + 1] if i + 1 < len(level) else level[i]
            parent = uint256_from_str(hash256(ser_uint256(left) + ser_uint256(right)))
            new_level.append(parent)
        # Collect the sibling at each level
        if index < len(level):
            sibling_index = index ^ 1
            if sibling_index < len(level):
                branch.append(level[sibling_index])
            else:
                branch.append(level[index])
        index >>= 1
        level = new_level
    return branch


def create_auxpow_block_from_template(node, chain_id, chain_height=4, nonce=7):
    """Create a valid AuxPoW block using getblocktemplate for MWEB compatibility.

    Adopted from wojakcore approach: miner constructs their own coinbase
    (no coinbasetxn in template response).

    Args:
        node: RPC node
        chain_id: auxpow chain ID
        chain_height: height of the chain merkle tree
        nonce: nonce for computing the expected index
    Returns:
        (block_hex, parent_hash) - hex-encoded block and parent block hash
    """
    template = node.getblocktemplate({"rules": ["segwit", "mweb"]})
    prev_hash = int(template['previousblockhash'], 16)
    nBits = int(template['bits'], 16)
    nTime = template['curtime']
    height = template['height']
    coinbase_value = template['coinbasevalue']

    child_version = VERSION_AUXPOW | (chain_id << 16) | 4

    # Build coinbase transaction (miner constructs their own, like wojakcore)
    coinbase_tx = CTransaction()
    coinbase_tx.vin = [CTxIn(COutPoint(0, 0xffffffff), script_BIP34_coinbase_height(height), 0xffffffff)]
    coinbase_tx.vout = [CTxOut(coinbase_value, CScript([OP_TRUE]))]

    # Add witness commitment if present
    if 'default_witness_commitment' in template:
        witness_script = CScript(bytes.fromhex(template['default_witness_commitment']))
        coinbase_tx.vout.append(CTxOut(0, witness_script))

    coinbase_tx.rehash()
    coinbase_txid = coinbase_tx.sha256

    # Build tx list: coinbase first, then template transactions
    tx_hex_list = [coinbase_tx.serialize_with_witness().hex()]
    txids = [coinbase_txid]
    for tx_obj in template['transactions']:
        tx_hex_list.append(tx_obj['data'])
        txids.append(int(tx_obj['txid'], 16))

    # Compute merkle root
    while len(txids) > 1:
        new_txids = []
        for i in range(0, len(txids), 2):
            left = ser_uint256(txids[i])
            right = ser_uint256(txids[i + 1] if i + 1 < len(txids) else txids[i])
            new_hash = uint256_from_str(hash256(left + right))
            new_txids.append(new_hash)
        txids = new_txids
    merkle_root = txids[0]

    # Build child block header
    child_header = CBlockHeader()
    child_header.nVersion = child_version
    child_header.hashPrevBlock = prev_hash
    child_header.hashMerkleRoot = merkle_root
    child_header.nTime = nTime
    child_header.nBits = nBits
    child_header.nNonce = 0
    child_header.calc_sha256()
    child_hash = child_header.sha256

    # Build chain merkle tree
    num_leaves = 1 << chain_height
    chain_index = get_expected_index(nonce, chain_id, chain_height)
    leaves = [child_hash if i == chain_index else 0 for i in range(num_leaves)]
    chain_branch = compute_merkle_branch(leaves, chain_index)
    chain_root = check_merkle_branch(child_hash, chain_branch, chain_index)

    # Build parent coinbase with merged mining data
    coinbase_data = bytearray()
    coinbase_data.extend(MERGED_MINING_HEADER)
    coinbase_data.extend(ser_uint256(chain_root)[::-1])
    coinbase_data.extend(struct.pack('<I', num_leaves))
    coinbase_data.extend(struct.pack('<I', nonce))

    parent_coinbase = CTransaction()
    parent_coinbase.vin = [CTxIn(COutPoint(0, 0xffffffff), CScript([coinbase_data]), 0xffffffff)]
    parent_coinbase.vout = [CTxOut(50 * COIN, CScript([OP_TRUE]))]
    parent_coinbase.rehash()

    # Build parent block
    import litecoin_scrypt
    parent_block = CBlock()
    parent_block.nVersion = 2
    parent_block.nTime = nTime - 1
    parent_block.nBits = nBits
    parent_block.nNonce = 0
    parent_block.vtx = [parent_coinbase]
    parent_block.hashMerkleRoot = parent_block.calc_merkle_root()

    # Mine parent block
    target = uint256_from_compact(parent_block.nBits)
    while True:
        parent_block.rehash()
        if parent_block.scrypt256 <= target:
            break
        parent_block.nNonce += 1

    parent_block.calc_sha256()

    # Build AuxPoW data
    auxpow_data = bytearray()
    tx_bytes = parent_coinbase.serialize_with_witness()
    auxpow_data.extend(tx_bytes)
    auxpow_data.extend(ser_uint256(parent_block.sha256))
    auxpow_data.extend(ser_compact_size(0))
    auxpow_data.extend(struct.pack('<i', 0))
    auxpow_data.extend(ser_compact_size(len(chain_branch)))
    for h in chain_branch:
        auxpow_data.extend(ser_uint256(h))
    auxpow_data.extend(struct.pack('<i', chain_index))
    parent_header = CBlockHeader(parent_block)
    auxpow_data.extend(parent_header.serialize())

    # Build full block
    block_data = bytearray()
    # Header
    block_data.extend(struct.pack('<i', child_header.nVersion))
    block_data.extend(ser_uint256(child_header.hashPrevBlock))
    block_data.extend(ser_uint256(child_header.hashMerkleRoot))
    block_data.extend(struct.pack('<I', child_header.nTime))
    block_data.extend(struct.pack('<I', child_header.nBits))
    block_data.extend(struct.pack('<I', child_header.nNonce))
    # AuxPoW data
    block_data.extend(auxpow_data)
    # Transactions
    block_data.extend(ser_compact_size(len(tx_hex_list)))
    for tx_hex in tx_hex_list:
        block_data.extend(bytes.fromhex(tx_hex))
    # MWEB block
    if 'mweb' in template:
        mweb_data = bytes.fromhex(template['mweb'])
        block_data.extend(struct.pack('B', 1))
        block_data.extend(mweb_data)
    else:
        block_data.extend(struct.pack('B', 0))

    return bytes(block_data), parent_block.hash


class AuxPowTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("=== Test 1: Verify block version bits at different heights ===")
        self.test_version_bits(node)

        self.log.info("=== Test 2: Verify AuxPoW activation at height 600 ===")
        self.test_auxpow_activation(node)

        self.log.info("=== Test 3: Submit valid AuxPoW block ===")
        self.test_valid_auxpow_block(node)

        self.log.info("=== Test 4: Submit AuxPoW block with wrong chain ID ===")
        self.test_wrong_chain_id_block(node)

        self.log.info("=== Test 5: Submit non-AuxPoW block after activation ===")
        self.test_non_auxpow_after_activation(node)

        self.log.info("All AuxPoW tests passed!")

    def test_version_bits(self, node):
        """Test that block versions have correct bits at different heights."""
        # Generate blocks to various heights and check versions
        block_hash = node.generate(1)[0]
        block = node.getblock(block_hash)
        v = block['version']
        self.log.info(f"  Height 1: version=0x{v:08x}, base={get_base_version(v)}, "
                      f"chainId=0x{get_chain_id(v):04x}, auxpow={is_auxpow(v)}, "
                      f"legacy={is_legacy(v)}")

        # Generate to just before SegWit
        node.generate(SEGWIT_HEIGHT - 1 - node.getblockcount())
        block_hash = node.getblockhash(SEGWIT_HEIGHT - 1)
        block = node.getblock(block_hash)
        v = block['version']
        self.log.info(f"  Height {SEGWIT_HEIGHT-1}: version=0x{v:08x}, base={get_base_version(v)}, "
                      f"chainId=0x{get_chain_id(v):04x}")

        # Generate to just before AuxPoW
        node.generate(AUXPOW_HEIGHT - 1 - node.getblockcount())
        block_hash = node.getblockhash(AUXPOW_HEIGHT - 1)
        block = node.getblock(block_hash)
        v = block['version']
        self.log.info(f"  Height {AUXPOW_HEIGHT-1}: version=0x{v:08x}, base={get_base_version(v)}, "
                      f"chainId=0x{get_chain_id(v):04x}, auxpow={is_auxpow(v)}")

        self.log.info("  Version bits test passed")

    def test_auxpow_activation(self, node):
        """Test that AuxPoW activates at the correct height."""
        # Generate to AuxPoW height
        current = node.getblockcount()
        if current < AUXPOW_HEIGHT:
            node.generate(AUXPOW_HEIGHT - current)

        block_hash = node.getblockhash(AUXPOW_HEIGHT)
        block = node.getblock(block_hash)
        v = block['version']
        self.log.info(f"  Height {AUXPOW_HEIGHT}: version=0x{v:08x}, base={get_base_version(v)}, "
                      f"chainId=0x{get_chain_id(v):04x}, auxpow={is_auxpow(v)}")

        # Verify chain ID is set
        assert_equal(get_chain_id(v), LNC_CHAIN_ID_BLOCK)
        # Non-AuxPoW block should not have VERSION_AUXPOW flag
        assert_equal(is_auxpow(v), False)

        # Verify one more block
        node.generate(1)
        block_hash = node.getblockhash(AUXPOW_HEIGHT + 1)
        block = node.getblock(block_hash)
        v = block['version']
        self.log.info(f"  Height {AUXPOW_HEIGHT+1}: version=0x{v:08x}, chainId=0x{get_chain_id(v):04x}")
        assert_equal(get_chain_id(v), LNC_CHAIN_ID_BLOCK)

        self.log.info("  AuxPoW activation test passed")

    def test_valid_auxpow_block(self, node):
        """Test submitting a valid AuxPoW block via submitblock."""
        tip_hash = node.getbestblockhash()
        tip = node.getblock(tip_hash)
        tip_hash_int = int(tip_hash, 16)

        self.log.info(f"  Current tip: height={tip['height']}, hash={tip_hash}")

        self.log.info("  Creating valid AuxPoW block from getblocktemplate...")
        auxpow_hex, parent_hash = create_auxpow_block_from_template(
            node,
            chain_id=LNC_CHAIN_ID_BLOCK,
            chain_height=4,
            nonce=7,
        )
        self.log.info(f"  Parent block hash: {parent_hash}")

        self.log.info("  Submitting AuxPoW block...")
        result = node.submitblock(bytes_to_hex_str(auxpow_hex))
        self.log.info(f"  submitblock result: {result}")

        new_tip = node.getbestblockhash()
        new_tip_block = node.getblock(new_tip)
        self.log.info(f"  New tip: height={new_tip_block['height']}, hash={new_tip}")

        assert_equal(new_tip_block['height'], tip['height'] + 1)

        v = new_tip_block['version']
        self.log.info(f"  Block version: 0x{v:08x}, auxpow={is_auxpow(v)}")
        assert_equal(is_auxpow(v), True)
        assert_equal(get_chain_id(v), LNC_CHAIN_ID_BLOCK)

        self.log.info("  Valid AuxPoW block test passed")

    def test_wrong_chain_id_block(self, node):
        """Test that AuxPoW block with wrong chain ID is rejected."""
        tip_hash = node.getbestblockhash()
        tip = node.getblock(tip_hash)

        wrong_chain_id = 0x1234
        self.log.info(f"  Creating AuxPoW block with wrong chain ID: 0x{wrong_chain_id:04x}")
        auxpow_hex, _ = create_auxpow_block_from_template(
            node,
            chain_id=wrong_chain_id,
            chain_height=4,
            nonce=7,
        )

        self.log.info("  Submitting AuxPoW block with wrong chain ID...")
        result = node.submitblock(bytes_to_hex_str(auxpow_hex))
        self.log.info(f"  submitblock result: {result}")

        # Block should be rejected — tip must not advance
        assert_equal(node.getbestblockhash(), tip_hash)
        self.log.info("  Wrong chain ID rejection test passed")

    def test_non_auxpow_after_activation(self, node):
        """Test that non-AuxPoW blocks are still accepted after activation."""
        tip_hash = node.getbestblockhash()
        tip = node.getblock(tip_hash)

        # Mine a non-AuxPoW block using generatetoaddress
        self.log.info("  Mining non-AuxPoW block after activation...")
        node.generatetoaddress(1, node.getnewaddress())
        node.syncwithvalidationinterfacequeue()

        new_tip = node.getbestblockhash()
        new_tip_block = node.getblock(new_tip)
        self.log.info(f"  New tip: height={new_tip_block['height']}, hash={new_tip}")

        assert_equal(new_tip_block['height'], tip['height'] + 1)

        v = new_tip_block['version']
        self.log.info(f"  Block version: 0x{v:08x}, auxpow={is_auxpow(v)}")
        assert_equal(is_auxpow(v), False)

        # Verify it's NOT an AuxPoW block
        v = new_tip_block['version']
        self.log.info(f"  Block version: 0x{v:08x}, auxpow={is_auxpow(v)}")
        assert_equal(is_auxpow(v), False)

        self.log.info("  Non-AuxPoW after activation test passed")


if __name__ == '__main__':
    AuxPowTest().main()
