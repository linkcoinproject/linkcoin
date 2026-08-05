// Copyright (c) 2014-2025 The Linkcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <consensus/params.h>
#include <primitives/block.h>

/** Linkcoin block subsidy: 80 LNC base, halving every 400k blocks, special IPO at height 10 */
CAmount GetLinkcoinBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);

/** Validate block subsidy */
bool ValidateBlockSubsidy(const CAmount nSubsidy, int nHeight, const Consensus::Params& consensusParams);
