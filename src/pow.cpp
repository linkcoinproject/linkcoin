// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2025 The Linkcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>
#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <cinttypes>
#include <primitives/block.h>
#include <timedata.h>
#include <uint256.h>
#include <util/system.h>

// Linkcoin EED difficulty parameters
static const int64_t nTargetSpacing = 4 * 60;       // 4 minutes
static const int64_t nTargetTimespan = 4 * 60;       // legacy: 4 mins per interval
static const int64_t nEEDHeight = 5000;
static const int64_t nEEDFixHeight = 5300;
static const int64_t nEEDV2Height = 5607;
static const int64_t nEEDHalflife = 172800;          // 48 hours (V0/V1)
static const int64_t nEEDV2Halflife = 28800;         // 8 hours (V2)
static const int64_t nEEDV3Height = 75000;           // V3 fix: per-block ASERT (hardfork 3)
static const int64_t nEEDV4Height = 75019;            // V4 fix: timestamp-stable ASERT (hardfork 4)
                                                     // Fixes bad-diffbits rejection when miners roll nTime,
                                                     // and raises the per-block cap from 4x to 64x for faster
                                                     // recovery after large hashrate drops.
static const int64_t nEEDV5Height = 76245;            // V5 fix: stall-breaking ASERT (hardfork 5)
                                                     // V4 uses prev solvetime only, so difficulty cannot
                                                     // adjust when no blocks are found (chain stalls).
                                                     // V5 uses the actual time gap (block.nTime - prev.nTime)
                                                     // when it exceeds 6x target spacing, allowing difficulty
                                                     // to drop and unstick the chain. Also fixes V4 overshoot
                                                     // by using 2-day halflife (like BCH) and 4x up cap.
static const int64_t nEEDV5NormalHalflife = 172800;   // 2 days — prevents overshoot (BCH uses this)
static const int64_t nEEDV5StallHalflife = 3600;      // 1 hour — fast emergency difficulty drop
static const int64_t nEEDV5StallThreshold = 6;        // gap > 6x target spacing (24 min) = stalled
static const int64_t nEEDV5CapUp = 4;                 // max 4x difficulty increase per block
static const int64_t nEEDV5CapDown = 64;              // max 64x difficulty decrease per block

static const int nClassic2Height = 2500;
static const int64_t nClassic2AveragingWindow = 24;
static const int64_t nClassic2MaxAdjustDown = 16;
static const int64_t nClassic2MaxAdjustUp = 8;
static const int64_t nClassic2DampeningFactor = 3;

static const int nDigiShieldFork = 388;

static const int64_t LEGACY_RETARGET_INTERVAL = 1;   // every block retarget (nInterval=1)

// BUGGY: uses addition/subtraction (blocks 5000-5299)
static arith_uint256 ApplyExponentialAdjustmentBuggy(const arith_uint256& target, int64_t nDrift, int64_t nHalflife)
{
    int64_t nCorrection = (nDrift * 45426) / nHalflife;
    arith_uint256 result = target;
    arith_uint256 adjustment;

    if (nCorrection >= 0) {
        adjustment = arith_uint256(nCorrection);
        adjustment = result * adjustment / arith_uint256(65536);
        result += adjustment;  // BUG: should multiply
    } else {
        adjustment = arith_uint256(-nCorrection);
        adjustment = result * adjustment / arith_uint256(65536);
        result -= adjustment;  // BUG: should multiply
    }
    return result;
}

// V1: integer shifts + linear fractional (blocks 5300-5606)
static arith_uint256 ApplyExponentialAdjustmentV1(const arith_uint256& target, int64_t nDrift, int64_t nHalflife)
{
    int64_t exponent = (nDrift * 65536) / nHalflife;
    int64_t shifts = exponent / 65536;
    int64_t frac = exponent % 65536;
    if (frac < 0) { shifts -= 1; frac += 65536; }

    arith_uint256 result = target;

    if (shifts >= 0) {
        if (shifts > 16) shifts = 16;
        for (int i = 0; i < shifts; i++) result *= 2;
    } else {
        int64_t neg_shifts = -shifts;
        if (neg_shifts > 16) neg_shifts = 16;
        for (int i = 0; i < neg_shifts; i++) result /= 2;
    }

    arith_uint256 fractional_adjustment = result * arith_uint256(frac);
    fractional_adjustment = fractional_adjustment * arith_uint256(45426) / arith_uint256(65536) / arith_uint256(65536);
    result += fractional_adjustment;

    return result;
}

// V2: Taylor polynomial 2^x (blocks 5607+)
static arith_uint256 ApplyExponentialAdjustmentV2(const arith_uint256& target, int64_t nDrift, int64_t nHalflife)
{
    static const int64_t nMaxDrift = 2592000;
    static const int64_t nMinDrift = -2592000;
    if (nDrift > nMaxDrift) nDrift = nMaxDrift;
    if (nDrift < nMinDrift) nDrift = nMinDrift;
    if (nHalflife <= 0) nHalflife = 1;

    int64_t nExponentFixed = (nDrift << 16) / nHalflife;
    int64_t nIntegerPart = nExponentFixed >> 16;
    int64_t nFractionalPart = nExponentFixed & 0xFFFF;
    bool bNegative = false;

    if (nExponentFixed < 0) {
        bNegative = true;
        nIntegerPart = (-nExponentFixed) >> 16;
        nFractionalPart = (-nExponentFixed) & 0xFFFF;
    }

    if (nIntegerPart > 3) { nIntegerPart = 3; nFractionalPart = 0; }

    int64_t x = nFractionalPart;
    int64_t x2 = (x * x) >> 16;
    int64_t x3 = (x2 * x) >> 16;
    int64_t x4 = (x3 * x) >> 16;
    int64_t x5 = (x4 * x) >> 16;
    int64_t x6 = (x5 * x) >> 16;

    int64_t nTwoToFrac = 65536;
    nTwoToFrac += (x * 45426) >> 16;
    nTwoToFrac += (x2 * 15743) >> 16;
    nTwoToFrac += (x3 * 3638) >> 16;
    nTwoToFrac += (x4 * 629) >> 16;
    nTwoToFrac += (x5 * 85) >> 16;
    nTwoToFrac += (x6 * 10) >> 16;
    if (nTwoToFrac <= 0) nTwoToFrac = 1;

    arith_uint256 result;

    if (bNegative) {
        // target / (2^intPart * 2^fracPart)
        result = (target * arith_uint256(65536)) / (arith_uint256(nTwoToFrac) << nIntegerPart);
    } else {
        // target * 2^intPart * 2^fracPart
        result = (target << nIntegerPart) * arith_uint256(nTwoToFrac) / arith_uint256(65536);
    }

    return result;
}

unsigned int DigiShield(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    // testnet special rule
    if (params.fPowAllowMinDifficultyBlocks && pblock) {
        if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
    }

    int64_t nBlocksToGoBack = 60;
    if (pindexLast->nHeight < nBlocksToGoBack)
        nBlocksToGoBack = pindexLast->nHeight;

    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nBlocksToGoBack; i++)
        pindexFirst = pindexFirst->pprev;
    if (!pindexFirst) return bnPowLimit.GetCompact();

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    int64_t nTargetTimespanCur = nBlocksToGoBack * nTargetSpacing;

    if (nActualTimespan < (nTargetTimespanCur - nTargetTimespanCur/4))
        nActualTimespan = nTargetTimespanCur - nTargetTimespanCur/4;
    if (nActualTimespan > (nTargetTimespanCur + nTargetTimespanCur/2))
        nActualTimespan = nTargetTimespanCur + nTargetTimespanCur/2;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespanCur;
    if (bnNew > bnPowLimit) bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

unsigned int Classic2AdaptiveDifficulty(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    // testnet special rule
    if (params.fPowAllowMinDifficultyBlocks && pblock) {
        if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
    }

    int64_t nTargetTimespanCur = nClassic2AveragingWindow * nTargetSpacing;

    const CBlockIndex* pindexFirst = pindexLast;
    int64_t nBlocksBack = 0;
    for (nBlocksBack = 0; nBlocksBack < nClassic2AveragingWindow && pindexFirst; nBlocksBack++) {
        if (!pindexFirst->pprev) break;
        pindexFirst = pindexFirst->pprev;
    }
    if (!pindexFirst || nBlocksBack < nClassic2AveragingWindow)
        return pindexLast->nBits;

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();

    if (pblock) {
        int64_t timeDiff = pblock->nTime - pindexLast->nTime;
        // Emergency rule: allow min difficulty at Classic2 activation
        if (pindexLast->nHeight == 2499 && timeDiff > nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
        if (timeDiff > nTargetSpacing * 4) return pindexLast->nBits;
        if (timeDiff < -60) return pindexLast->nBits;
    }

    int64_t nAdjustedTimespan = nTargetTimespanCur + (nActualTimespan - nTargetTimespanCur) / nClassic2DampeningFactor;
    int64_t nMinTimespan = nTargetTimespanCur * (100 - nClassic2MaxAdjustUp) / 100;
    int64_t nMaxTimespan = nTargetTimespanCur * (100 + nClassic2MaxAdjustDown) / 100;

    if (nAdjustedTimespan < nMinTimespan) nAdjustedTimespan = nMinTimespan;
    if (nAdjustedTimespan > nMaxTimespan) nAdjustedTimespan = nMaxTimespan;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nAdjustedTimespan;
    bnNew /= nTargetTimespanCur;

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;

    // Safety: max 50% drop per adjustment
    arith_uint256 bnLastTarget;
    bnLastTarget.SetCompact(pindexLast->nBits);
    arith_uint256 bnMaxDrop = (bnLastTarget * arith_uint256(3)) / arith_uint256(2);
    if (bnNew > bnMaxDrop) bnNew = bnMaxDrop;

    return bnNew.GetCompact();
}

unsigned int ElasticExponentialDifficultyBuggy(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    int64_t nHalflife = nEEDHalflife;
    const CBlockIndex* pindexPrevPrev = pindexLast->pprev;
    if (!pindexPrevPrev) return bnPowLimit.GetCompact();

    int64_t nSolvetime = pindexLast->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (nSolvetime < 1) nSolvetime = 1;

    int64_t nDrift = nSolvetime - nTargetSpacing;

    bool fStress = false;
    if (nSolvetime > nTargetSpacing * 4) fStress = true;
    if (nSolvetime < nTargetSpacing / 4) fStress = true;
    if (fStress) nHalflife /= 4;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew = ApplyExponentialAdjustmentBuggy(bnNew, nDrift, nHalflife);

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;
    if (bnNew == 0) bnNew = arith_uint256(1);

    return bnNew.GetCompact();
}

unsigned int ElasticExponentialDifficultyV1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    int64_t nHalflife = nEEDHalflife;
    const CBlockIndex* pindexPrevPrev = pindexLast->pprev;
    if (!pindexPrevPrev) return bnPowLimit.GetCompact();

    int64_t nSolvetime = pindexLast->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (nSolvetime < 1) nSolvetime = 1;

    int64_t nDrift = nSolvetime - nTargetSpacing;

    bool fStress = false;
    if (nSolvetime > nTargetSpacing * 4) fStress = true;
    if (nSolvetime < nTargetSpacing / 4) fStress = true;
    if (fStress) nHalflife /= 4;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew = ApplyExponentialAdjustmentV1(bnNew, nDrift, nHalflife);

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;
    if (bnNew == 0) bnNew = arith_uint256(1);

    return bnNew.GetCompact();
}

unsigned int ElasticExponentialDifficultyV2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    // testnet: min-difficulty block after 2x target spacing
    if (params.fPowAllowMinDifficultyBlocks && pblock) {
        if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
    }

    int64_t nHalflife = nEEDV2Halflife;

    int64_t nCurrentTime;
    if (pblock) nCurrentTime = pblock->nTime;
    else nCurrentTime = GetAdjustedTime();

    int64_t nTimeSinceLastBlock = nCurrentTime - pindexLast->GetBlockTime();
    if (nTimeSinceLastBlock < 1) nTimeSinceLastBlock = 1;

    // Anchor-based ASERT: find the V2 activation block
    static const CBlockIndex* pindexCachedAnchor = nullptr;
    static const CBlockIndex* pindexVerifiedTip = nullptr;
    static int64_t nCachedAnchorTime = 0;
    static unsigned int nCachedAnchorBits = 0;

    const CBlockIndex* pindexAnchor = pindexCachedAnchor;
    bool bNeedRefresh = (!pindexAnchor || pindexAnchor->nHeight != nEEDV2Height);

    if (!bNeedRefresh && pindexAnchor) {
        if (pindexLast != pindexVerifiedTip && pindexLast->pprev != pindexVerifiedTip) {
            const CBlockIndex* ptest = pindexLast;
            while (ptest && ptest->nHeight > nEEDV2Height) ptest = ptest->pprev;
            if (ptest != pindexAnchor) { bNeedRefresh = true; pindexVerifiedTip = nullptr; }
            else pindexVerifiedTip = pindexLast;
        } else pindexVerifiedTip = pindexLast;
    }

    if (bNeedRefresh) {
        pindexAnchor = pindexLast;
        while (pindexAnchor && pindexAnchor->nHeight > nEEDV2Height)
            pindexAnchor = pindexAnchor->pprev;
        if (pindexAnchor && pindexAnchor->nHeight >= nEEDV2Height) {
            pindexCachedAnchor = pindexAnchor;
            nCachedAnchorTime = pindexAnchor->GetBlockTime();
            nCachedAnchorBits = pindexAnchor->nBits;
        }
    }

    int64_t nDrift;
    arith_uint256 bnAnchorTarget;

    if (pindexAnchor && pindexAnchor->nHeight >= nEEDV2Height) {
        int64_t nHeightDiff = (pindexLast->nHeight + 1) - pindexAnchor->nHeight;
        int64_t nTargetTime = nHeightDiff * nTargetSpacing;
        int64_t nActualTime = nCurrentTime - (nCachedAnchorTime ? nCachedAnchorTime : pindexAnchor->GetBlockTime());
        nDrift = nActualTime - nTargetTime;
        bnAnchorTarget.SetCompact(nCachedAnchorBits ? nCachedAnchorBits : pindexAnchor->nBits);
    } else {
        nDrift = nTimeSinceLastBlock - nTargetSpacing;
        bnAnchorTarget.SetCompact(pindexLast->nBits);
    }

    if (nHalflife < 1) nHalflife = 1;

    arith_uint256 bnNew = ApplyExponentialAdjustmentV2(bnAnchorTarget, nDrift, nHalflife);

    // Cap max 4x difficulty increase
    arith_uint256 bnLastTarget;
    bnLastTarget.SetCompact(pindexLast->nBits);
    arith_uint256 bnMinTarget = bnLastTarget / arith_uint256(4);
    if (bnNew < bnMinTarget) {
        bnNew = bnMinTarget;
        LogPrintf("EED V2: Capped difficulty increase to 4x\n");
    }

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;
    if (bnNew == 0) bnNew = arith_uint256(1);

    if (nTimeSinceLastBlock > nTargetSpacing * 2 || (pindexLast->nHeight + 1) % 100 == 0) {
        LogPrintf("EED V2: height=%d, timeSinceLast=%" PRId64 "s, drift=%" PRId64 "s, halflife=%" PRId64 "s\n",
                  pindexLast->nHeight + 1, nTimeSinceLastBlock, nDrift, nHalflife);
    }

    return bnNew.GetCompact();
}

// EED V3: Per-block ASERT (blocks 66000+)
// Fixes the V2 anchor-based cap bug where difficulty was limited to 8x the anchor.
// Uses previous-block-based formula: target * 2^((solvetime - target_spacing) / halflife)
unsigned int ElasticExponentialDifficultyV3(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    if (params.fPowAllowMinDifficultyBlocks && pblock) {
        if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
    }

    int64_t nHalflife = nEEDV2Halflife;

    int64_t nCurrentTime;
    if (pblock) nCurrentTime = pblock->nTime;
    else nCurrentTime = GetAdjustedTime();

    int64_t nSolvetime = nCurrentTime - pindexLast->GetBlockTime();
    if (nSolvetime < 1) nSolvetime = 1;

    // Per-block drift: how much this single block deviated from target
    int64_t nDrift = nSolvetime - nTargetSpacing;

    if (nHalflife < 1) nHalflife = 1;

    // Apply exponential adjustment relative to PREVIOUS block's target (not anchor)
    arith_uint256 bnPrevTarget;
    bnPrevTarget.SetCompact(pindexLast->nBits);
    arith_uint256 bnNew = ApplyExponentialAdjustmentV2(bnPrevTarget, nDrift, nHalflife);

    // Safety cap: max 4x difficulty change per block
    arith_uint256 bnMinTarget = bnPrevTarget / arith_uint256(4);
    arith_uint256 bnMaxTarget = bnPrevTarget * arith_uint256(4);

    if (bnNew < bnMinTarget) bnNew = bnMinTarget;
    if (bnNew > bnMaxTarget) bnNew = bnMaxTarget;

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;
    if (bnNew == 0) bnNew = arith_uint256(1);

    return bnNew.GetCompact();
}

// EED V4: Timestamp-stable ASERT (blocks 75019+)
//
// Fixes two V3 problems that caused the chain to stall at block 75018:
//
//   1. bad-diffbits rejection when miners roll nTime.
//      V3 computes the target from (block.nTime - prev.nTime), so the target
//      depends on the very timestamp the miner is free to choose.  With the
//      4x per-block cap, the bits returned for one timestamp are rejected for
//      another timestamp in the same template, so almost every rolled block
//      fails ContextualCheckBlockHeader with "bad-diffbits".
//
//   2. 4x cap too tight for recovery.
//      When hashrate drops by 100x, V3 needs ~4 blocks (each ~6.7h) to catch
//      up, during which the chain is effectively frozen.
//
// V4 changes:
//   - The target is computed from the PREVIOUS block's solvetime only
//     (prev.nTime - prevprev.nTime), so it is fixed for the block being mined
//     and does not change when the miner rolls nTime.  This is the standard
//     ASERT formulation and matches what ContextualCheckBlockHeader expects.
//   - The per-block cap is raised to 64x (6 bits) so the difficulty can drop
//     fast enough to clear a backlog within a few blocks.
//   - Halflife stays at 8 hours (nEEDV2Halflife); the formula is otherwise
//     identical to V3.
unsigned int ElasticExponentialDifficultyV4(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    // testnet/regtest: min-difficulty block after 2x target spacing
    if (params.fPowAllowMinDifficultyBlocks && pblock) {
        if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
    }

    const int64_t nHalflife = nEEDV2Halflife;

    // Use the previous block's solvetime (prev - prevprev), NOT the block
    // being mined.  This makes the target independent of nTime and therefore
    // stable across time-rolling by the miner.
    int64_t nSolvetime = nTargetSpacing;  // default for height 1 / no parent
    if (pindexLast->pprev != nullptr) {
        nSolvetime = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();
    }
    if (nSolvetime < 1) nSolvetime = 1;

    int64_t nDrift = nSolvetime - nTargetSpacing;

    arith_uint256 bnPrevTarget;
    bnPrevTarget.SetCompact(pindexLast->nBits);
    arith_uint256 bnNew = ApplyExponentialAdjustmentV2(bnPrevTarget, nDrift, nHalflife);

    // Safety cap: max 64x difficulty change per block (was 4x in V3).
    // 64x lets a 100x hashrate drop recover in ~2 blocks instead of ~4.
    arith_uint256 bnMinTarget = bnPrevTarget / arith_uint256(64);
    arith_uint256 bnMaxTarget = bnPrevTarget * arith_uint256(64);

    if (bnNew < bnMinTarget) bnNew = bnMinTarget;
    if (bnNew > bnMaxTarget) bnNew = bnMaxTarget;

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;
    if (bnNew == 0) bnNew = arith_uint256(1);

    return bnNew.GetCompact();
}

// EED V5: Stall-breaking ASERT with BCH-style parameters (blocks 76245+)
//
// Fixes two V4 problems:
//
//   1. Chain stalls when no blocks are found.
//      V4 computes target from prev solvetime only (prev.nTime - prevprev.nTime),
//      so the target is fixed regardless of how long it takes to find the next
//      block. If difficulty is too high and no miner is running, the chain
//      stalls forever.
//
//   2. Difficulty overshoot during recovery.
//      V4 uses 8-hour halflife + 64x cap for BOTH up and down. When blocks come
//      fast (e.g. 1-2s), difficulty rises exponentially and overshoots, then the
//      chain stalls at the overshoot difficulty.
//
// V5 changes:
//   - Normal operation (gap <= 24 min):
//     ASERT with 2-day halflife (like BCH) and 4x up / 64x down caps.
//     The 2-day halflife prevents overshoot: at 1s solvetime, difficulty rises
//     only 0.1% per block (vs V4's potential 64x per block).
//   - Stall-breaking (gap > 24 min):
//     ASERT with 1-hour halflife and 64x cap, using the ACTUAL time gap.
//     After 1h stall: difficulty drops ~2x. After 6h: drops ~64x.
//   - The final target is the MAXIMUM (easiest) of normal and stall-breaker.
//   - ContextualCheckBlockHeader accepts targets in [V5 base, V5 expected],
//     preventing bad-diffbits while limiting attacker advantage.
//
unsigned int ElasticExponentialDifficultyV5(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();

    // testnet/regtest: min-difficulty block after 2x target spacing
    if (params.fPowAllowMinDifficultyBlocks && pblock) {
        if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
            return bnPowLimit.GetCompact();
    }

    arith_uint256 bnPrevTarget;
    bnPrevTarget.SetCompact(pindexLast->nBits);

    // --- Normal target: ASERT with 2-day halflife, 4x up / 64x down caps ---
    // Uses prev solvetime (timestamp-stable, no bad-diffbits)
    int64_t nSolvetime = nTargetSpacing;  // default for height 1 / no parent
    if (pindexLast->pprev != nullptr)
        nSolvetime = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();
    if (nSolvetime < 1) nSolvetime = 1;

    int64_t nNormalDrift = nSolvetime - nTargetSpacing;
    arith_uint256 bnNormalTarget = ApplyExponentialAdjustmentV2(
        bnPrevTarget, nNormalDrift, nEEDV5NormalHalflife);

    // Asymmetric caps: 4x up (prevent overshoot), 64x down (fast recovery)
    arith_uint256 bnMinTargetNormal = bnPrevTarget / arith_uint256(nEEDV5CapUp);
    arith_uint256 bnMaxTargetNormal = bnPrevTarget * arith_uint256(nEEDV5CapDown);
    if (bnNormalTarget < bnMinTargetNormal) bnNormalTarget = bnMinTargetNormal;
    if (bnNormalTarget > bnMaxTargetNormal) bnNormalTarget = bnMaxTargetNormal;
    if (bnNormalTarget > bnPowLimit) bnNormalTarget = bnPowLimit;
    if (bnNormalTarget == 0) bnNormalTarget = arith_uint256(1);

    // Without block header, can't compute time gap — return normal target
    if (!pblock) return bnNormalTarget.GetCompact();

    // --- Check if stall-breaking is needed ---
    int64_t nTimeSinceLastBlock = pblock->nTime - pindexLast->GetBlockTime();

    // Normal case: gap <= stall threshold (24 min)
    if (nTimeSinceLastBlock <= nTargetSpacing * nEEDV5StallThreshold)
        return bnNormalTarget.GetCompact();

    // --- Stall-breaking: ASERT with 1-hour halflife, 64x cap ---
    if (nTimeSinceLastBlock < 1) nTimeSinceLastBlock = 1;

    int64_t nStallDrift = nTimeSinceLastBlock - nTargetSpacing;
    arith_uint256 bnStallTarget = ApplyExponentialAdjustmentV2(
        bnPrevTarget, nStallDrift, nEEDV5StallHalflife);

    // 64x cap both directions for stall-breaking
    arith_uint256 bnMinTargetStall = bnPrevTarget / arith_uint256(64);
    arith_uint256 bnMaxTargetStall = bnPrevTarget * arith_uint256(64);
    if (bnStallTarget < bnMinTargetStall) bnStallTarget = bnMinTargetStall;
    if (bnStallTarget > bnMaxTargetStall) bnStallTarget = bnMaxTargetStall;
    if (bnStallTarget > bnPowLimit) bnStallTarget = bnPowLimit;
    if (bnStallTarget == 0) bnStallTarget = arith_uint256(1);

    // Return the EASIER target (higher target = lower difficulty)
    if (bnStallTarget > bnNormalTarget) {
        LogPrintf("EED V5: stall-breaking at height=%d, timeGap=%" PRId64 "s (%.1fh), "
                  "normal_diff→stall_diff drop\n",
                  pindexLast->nHeight + 1, nTimeSinceLastBlock,
                  (double)nTimeSinceLastBlock / 3600.0);
        return bnStallTarget.GetCompact();
    }

    return bnNormalTarget.GetCompact();
}

// V5 helper: compute the V5 normal base target (timestamp-stable, no stall-breaking)
// Used by ContextualCheckBlockHeader to validate V5+ blocks.
unsigned int GetV5BaseTarget(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    if (pindexLast == nullptr) return UintToArith256(params.powLimit).GetCompact();

    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnPrevTarget;
    bnPrevTarget.SetCompact(pindexLast->nBits);

    int64_t nSolvetime = nTargetSpacing;
    if (pindexLast->pprev != nullptr)
        nSolvetime = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();
    if (nSolvetime < 1) nSolvetime = 1;

    int64_t nDrift = nSolvetime - nTargetSpacing;
    arith_uint256 bnTarget = ApplyExponentialAdjustmentV2(
        bnPrevTarget, nDrift, nEEDV5NormalHalflife);

    arith_uint256 bnMinTarget = bnPrevTarget / arith_uint256(nEEDV5CapUp);
    arith_uint256 bnMaxTarget = bnPrevTarget * arith_uint256(nEEDV5CapDown);
    if (bnTarget < bnMinTarget) bnTarget = bnMinTarget;
    if (bnTarget > bnMaxTarget) bnTarget = bnMaxTarget;
    if (bnTarget > bnPowLimit) bnTarget = bnPowLimit;
    if (bnTarget == 0) bnTarget = arith_uint256(1);

    return bnTarget.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);

    if (pindexLast == nullptr)
        return bnPowLimit.GetCompact();

    // Default rule for chains with no retargeting (regtest, low-difficulty testnet)
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    int nHeight = pindexLast->nHeight + 1;

    // EED V5 - block 76245+ (stall-breaking ASERT)
    if (nHeight >= nEEDV5Height)
        return ElasticExponentialDifficultyV5(pindexLast, pblock, params);

    // EED V4 - block 75019+ (timestamp-stable ASERT, 64x cap)
    if (nHeight >= nEEDV4Height)
        return ElasticExponentialDifficultyV4(pindexLast, pblock, params);

    // EED V3 - block 66000+ (fixed per-block ASERT, no anchor cap)
    if (nHeight >= nEEDV3Height)
        return ElasticExponentialDifficultyV3(pindexLast, pblock, params);

    // EED V2 - block 5607+ (anchor-based, has cap bug)
    if (nHeight >= nEEDV2Height)
        return ElasticExponentialDifficultyV2(pindexLast, pblock, params);

    // EED V1 - blocks 5300-5606
    if (nHeight >= nEEDFixHeight)
        return ElasticExponentialDifficultyV1(pindexLast, pblock, params);

    // EED Buggy - blocks 5000-5299
    if (nHeight >= nEEDHeight)
        return ElasticExponentialDifficultyBuggy(pindexLast, pblock, params);

    // Classic2 Adaptive - block 2500+
    if (nHeight >= nClassic2Height)
        return Classic2AdaptiveDifficulty(pindexLast, pblock, params);

    // DigiShield - block 388+
    if (nHeight >= nDigiShieldFork)
        return DigiShield(pindexLast, pblock, params);

    // Original retarget (blocks 0-387)
    if (nHeight % LEGACY_RETARGET_INTERVAL != 0) {
        if (params.fPowAllowMinDifficultyBlocks) {
            if (pblock && pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
                return bnPowLimit.GetCompact();
            else {
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % LEGACY_RETARGET_INTERVAL != 0 && pindex->nBits == bnPowLimit.GetCompact())
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    int blockstogoback = LEGACY_RETARGET_INTERVAL - 1;
    if (nHeight != LEGACY_RETARGET_INTERVAL)
        blockstogoback = LEGACY_RETARGET_INTERVAL;

    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    if (nActualTimespan < nTargetTimespan/4) nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4) nActualTimespan = nTargetTimespan*4;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
    if (bnNew > bnPowLimit) bnNew = bnPowLimit;

    LogPrintf("GetNextWorkRequired RETARGET: height=%d, Before=%08x, After=%08x\n",
              nHeight, pindexLast->nBits, bnNew.GetCompact());

    return bnNew.GetCompact();
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

// Linkcoin: Use AuxPoW-aware PoW check (handles legacy + future AuxPoW blocks)
bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    return CheckAuxPowProofOfWork(block, params);
}
