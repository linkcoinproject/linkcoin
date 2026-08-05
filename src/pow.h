// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2025 The Linkcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LINKCOIN_POW_H
#define LINKCOIN_POW_H

#include <consensus/params.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;
class arith_uint256;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);
/** AuxPoW-aware Proof-of-Work check for block headers */
bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params);
bool CheckAuxPowProofOfWork(const CBlockHeader& block, const Consensus::Params& params);

// Linkcoin EED difficulty adjustment functions
unsigned int DigiShield(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);
unsigned int Classic2AdaptiveDifficulty(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);
unsigned int ElasticExponentialDifficultyBuggy(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);
unsigned int ElasticExponentialDifficultyV1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);
unsigned int ElasticExponentialDifficultyV2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);
unsigned int ElasticExponentialDifficultyV3(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);

#endif // LINKCOIN_POW_H
