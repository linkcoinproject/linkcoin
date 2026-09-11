// Copyright (c) 2014-2025 The Linkcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <auxpow.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <pow.h>
#include <primitives/block.h>
#include <util/system.h>
#include <validation.h>
#include <versionbits.h>

// Linkcoin block subsidy: 80 LNC, halving every 400,000 blocks
// Special IPO block at height 10: 500,000 LNC
CAmount GetLinkcoinBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
    CAmount nSubsidy = 80 * COIN;
    if (nHeight == 10) nSubsidy = 500000 * COIN;

    nSubsidy >>= (nHeight / 400000);

    return nSubsidy;
}

bool ValidateBlockSubsidy(const CAmount nSubsidy, int nHeight, const Consensus::Params& consensusParams)
{
    CAmount expectedSubsidy = GetLinkcoinBlockSubsidy(nHeight, consensusParams);
    return nSubsidy <= expectedSubsidy;
}

// AuxPoW-aware proof-of-work check (supports future AuxPoW activation)
bool CheckAuxPowProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    // Non-AuxPoW blocks: just check PoW
    // Chain ID is not validated for non-AuxPoW blocks because BIP9 version
    // bits (29-31) overlap with the chain ID field in the version.
    // For AuxPoW blocks, chain ID is validated by auxpow->check() below.
    if (!block.auxpow) {
        if (block.IsAuxpow()) {
            return error("%s: no auxpow on block with auxpow version", __func__);
        }
        if (block.GetHash() == params.hashGenesisBlock && block.IsLegacy()) {
            return true;
        }
        return CheckProofOfWork(block.GetPoWHash(), block.nBits, params);
    }

    // AuxPoW blocks
    if (!block.IsAuxpow()) {
        return error("%s: auxpow on block with non-auxpow version", __func__);
    }
    // Validate that the block version's chain ID matches the network's expected chain ID
    // GetChainId() returns nVersion >> 16 which includes VERSIONBITS_TOP_BITS (0x2000)
    int32_t nExpectedChainId = params.nAuxpowChainId | (VERSIONBITS_TOP_BITS >> 16);
    if (block.GetChainId() != nExpectedChainId) {
        return error("%s: block chain ID %x does not match expected %x", __func__,
                     block.GetChainId(), nExpectedChainId);
    }
    if (!block.auxpow->check(block.GetHash(), block.GetChainId(), params)) {
        return error("%s: AUX POW is not valid", __func__);
    }
    return CheckProofOfWork(block.auxpow->getParentBlockPoWHash(), block.nBits, params);
}
