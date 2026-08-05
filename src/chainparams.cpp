// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2014-2025 The Linkcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>
#include <script/standard.h>
#include <key_io.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block.
 *
 * Linkcoin mainnet genesis:
 * CBlock(hash=2865bdde500f4c65eb97ac69c9bc29850a0571a19f2121613c283fbd2d334bd7,
 *        ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e1a7071da2ffc19a27b79029c110fa238b54d6c46933e5c5fb29cdca838d170e,
 *        nTime=1406314882, nBits=1e0ffff0, nNonce=1567578, vtx=1)
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "(2014-06-15 T 12:05 UTC) Coverage of the Group E match between Switzerland and Ecuador at the 2014 WorldCup.";
    const CScript genesisOutputScript = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 400000; // Linkcoin: halving every 400k blocks
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x2865bdde500f4c65eb97ac69c9bc29850a0571a19f2121613c283fbd2d334bd7");
        consensus.BIP65Height = 99999999; // Disabled
        consensus.BIP66Height = 99999999; // Disabled
        consensus.CSVHeight = std::numeric_limits<int>::max(); // Disabled
        consensus.SegwitHeight = std::numeric_limits<int>::max(); // Disabled
        consensus.TaprootHeight = std::numeric_limits<int>::max(); // Disabled
        consensus.DisabledScriptReactivationHeight = std::numeric_limits<int>::max(); // Disabled
        consensus.MWEBHeight = std::numeric_limits<int>::max(); // Disabled
        consensus.MinBIP9WarningHeight = 10080 + 10080;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 4 * 60;      // Linkcoin: 4 minutes
        consensus.nPowTargetSpacing = 4 * 60;        // Linkcoin: 4 minutes
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 9576;
        consensus.nMinerConfirmationWindow = 10080;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;

        consensus.vDeployments[Consensus::DEPLOYMENT_BIP34].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP34].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP34].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.vDeployments[Consensus::DEPLOYMENT_BIP66].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP66].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP66].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.vDeployments[Consensus::DEPLOYMENT_BIP65].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP65].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_BIP65].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartHeight = std::numeric_limits<int>::max();
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeoutHeight = std::numeric_limits<int>::max();

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartHeight = std::numeric_limits<int>::max();
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeoutHeight = std::numeric_limits<int>::max();

        consensus.vDeployments[Consensus::DEPLOYMENT_MWEB].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_MWEB].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_MWEB].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_MWEB].nStartHeight = std::numeric_limits<int>::max();
        consensus.vDeployments[Consensus::DEPLOYMENT_MWEB].nTimeoutHeight = std::numeric_limits<int>::max();

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.defaultAssumeValid = uint256S("0x2865bdde500f4c65eb97ac69c9bc29850a0571a19f2121613c283fbd2d334bd7");

        // AuxPoW parameters - disabled until future activation
        consensus.nAuxpowChainId = 0x4C4E; // "LN" in hex - Linkcoin chain ID
        consensus.fStrictChainId = true;
        consensus.nAuxpowStartHeight = std::numeric_limits<int>::max(); // Disabled for now
        consensus.nBlockAfterAuxpowRewardThreshold = 5;
        consensus.nLegacyBlocksBefore = -1; // Always allow legacy blocks

        consensus.nHeightEffective = 0;
        consensus.nCoinbaseMaturity = 100; // Linkcoin: 100 blocks

        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xc0;
        pchMessageStart[2] = 0xb6;
        pchMessageStart[3] = 0xdb;
        nDefaultPort = 7200;    // Linkcoin P2P port
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1406314882, 1567578, 0x1e0ffff0, 1, 80 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x2865bdde500f4c65eb97ac69c9bc29850a0571a19f2121613c283fbd2d334bd7"));
        assert(genesis.hashMerkleRoot == uint256S("0xe1a7071da2ffc19a27b79029c110fa238b54d6c46933e5c5fb29cdca838d170e"));

        vSeeds.clear();
        vSeeds.emplace_back("103.133.25.201");
        vSeeds.emplace_back("159.223.90.59");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,48);   // Linkcoin: addresses start with 'L'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);    // Script addresses start with '3'
        base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,176);  // WIF keys start with 'Q'
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "lnc";
        mweb_hrp = "lncmweb";

        vFixedSeeds.clear();

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 0,     uint256S("0x2865bdde500f4c65eb97ac69c9bc29850a0571a19f2121613c283fbd2d334bd7")},
                { 388,   uint256S("0x28364d696c6ef3ce6e60440a78b47c044279dfe5f83baae2d8b683635a9111e3")},
                { 1000,  uint256S("0xe2532cd019b6faa2bc61048c4697dfb9c179f1a08ba851a80b18dfcf42ba2375")},
                { 1500,  uint256S("0x70ded398ea3211ac6b8658a5af23dd30a981008d24e8bfa357d7d6bfa36e31d5")},
                { 2000,  uint256S("0x001ac43f8eac777f553416c5bd917f4cf3faa01fe93a29d5bb535d3681071498")},
                { 2500,  uint256S("0x91c57733327078bb92d61baf5a084de801f60aa1cfec5094896a22eb149c9bce")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        // Linkcoin: no development fund
        vDevelopmentFundAddress.clear();
        vDevelopmentFundStartHeight = std::numeric_limits<int>::max();
        vDevelopmentFundLastHeight = 0;
        vDevelopmentFundPercent = 0.0;

        // Consensus tree
        digishieldConsensus = consensus;
        digishieldConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();
        digishieldConsensus.fSimplifiedRewards = false;
        digishieldConsensus.fDigishieldDifficultyCalculation = false;
        digishieldConsensus.nPowTargetTimespan = 4 * 60;
        digishieldConsensus.nPowTargetSpacing = 4 * 60;

        minDifficultyConsensus = digishieldConsensus;
        minDifficultyConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();
        minDifficultyConsensus.fPowAllowDigishieldMinDifficultyBlocks = false;
        minDifficultyConsensus.fPowAllowMinDifficultyBlocks = true;

        auxpowConsensus = digishieldConsensus;
        auxpowConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();

        pConsensusRoot = &digishieldConsensus;
        digishieldConsensus.pLeft = &consensus;
        digishieldConsensus.pRight = &auxpowConsensus;
    }
};

/**
 * Testnet
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 400000;
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x08303e4552b32f1451c7a187f3ba0bb6d8d85ebce6e1645587f5408a64877d1f");
        consensus.BIP65Height = 99999999;
        consensus.BIP66Height = 99999999;
        consensus.CSVHeight = std::numeric_limits<int>::max();
        consensus.SegwitHeight = std::numeric_limits<int>::max();
        consensus.TaprootHeight = std::numeric_limits<int>::max();
        consensus.DisabledScriptReactivationHeight = std::numeric_limits<int>::max();
        consensus.MWEBHeight = std::numeric_limits<int>::max();
        consensus.MinBIP9WarningHeight = 10080 + 10080;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 4 * 60;
        consensus.nPowTargetSpacing = 4 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 9576;
        consensus.nMinerConfirmationWindow = 10080;

        consensus.nAuxpowChainId = 0x4C4E;
        consensus.fStrictChainId = true;
        consensus.nAuxpowStartHeight = std::numeric_limits<int>::max();
        consensus.nBlockAfterAuxpowRewardThreshold = 0;
        consensus.nLegacyBlocksBefore = -1;

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.defaultAssumeValid = uint256S("0x08303e4552b32f1451c7a187f3ba0bb6d8d85ebce6e1645587f5408a64877d1f");
        consensus.nCoinbaseMaturity = 30;

        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdc;
        nDefaultPort = 72555;   // Linkcoin testnet P2P port
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1404911137, 402647, 0x1e0ffff0, 1, 80 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x08303e4552b32f1451c7a187f3ba0bb6d8d85ebce6e1645587f5408a64877d1f"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tlnc";
        mweb_hrp = "tlncmweb";

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 0, uint256S("0x08303e4552b32f1451c7a187f3ba0bb6d8d85ebce6e1645587f5408a64877d1f")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        vDevelopmentFundAddress.clear();
        vDevelopmentFundStartHeight = std::numeric_limits<int>::max();
        vDevelopmentFundLastHeight = 0;
        vDevelopmentFundPercent = 0.0;

        // No consensus tree needed for Linkcoin testnet
        digishieldConsensus = consensus;
        digishieldConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();
        digishieldConsensus.fSimplifiedRewards = false;
        digishieldConsensus.fDigishieldDifficultyCalculation = false;
        digishieldConsensus.nPowTargetTimespan = 4 * 60;
        digishieldConsensus.nPowTargetSpacing = 4 * 60;

        minDifficultyConsensus = digishieldConsensus;
        minDifficultyConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();
        minDifficultyConsensus.fPowAllowDigishieldMinDifficultyBlocks = false;
        minDifficultyConsensus.fPowAllowMinDifficultyBlocks = true;

        auxpowConsensus = digishieldConsensus;
        auxpowConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();

        pConsensusRoot = &digishieldConsensus;
        digishieldConsensus.pLeft = &consensus;
        digishieldConsensus.pRight = &auxpowConsensus;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 400000;
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x00");
        consensus.BIP65Height = 99999999;
        consensus.BIP66Height = 99999999;
        consensus.CSVHeight = 0;
        consensus.SegwitHeight = 0;
        consensus.TaprootHeight = 0;
        consensus.DisabledScriptReactivationHeight = 0;
        consensus.MWEBHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 4 * 60;
        consensus.nPowTargetSpacing = 4 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108;
        consensus.nMinerConfirmationWindow = 144;

        consensus.nAuxpowChainId = 0;
        consensus.fStrictChainId = false;
        consensus.nAuxpowStartHeight = 0;
        consensus.nBlockAfterAuxpowRewardThreshold = 0;
        consensus.nLegacyBlocksBefore = -1;

        consensus.nMinimumChainWork = uint256S("0x00");
        consensus.defaultAssumeValid = uint256S("0x00");
        consensus.nCoinbaseMaturity = 30;

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SCRIPT_ADDRESS2] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "rlnc";
        mweb_hrp = "rlncmweb";

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                // No checkpoints for regtest
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        vDevelopmentFundAddress.clear();
        vDevelopmentFundStartHeight = std::numeric_limits<int>::max();
        vDevelopmentFundLastHeight = 0;
        vDevelopmentFundPercent = 0.0;

        digishieldConsensus = consensus;
        digishieldConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();
        digishieldConsensus.fSimplifiedRewards = false;
        digishieldConsensus.fDigishieldDifficultyCalculation = false;
        digishieldConsensus.nPowTargetTimespan = 4 * 60;
        digishieldConsensus.nPowTargetSpacing = 4 * 60;

        minDifficultyConsensus = digishieldConsensus;
        minDifficultyConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();
        minDifficultyConsensus.fPowAllowDigishieldMinDifficultyBlocks = false;
        minDifficultyConsensus.fPowAllowMinDifficultyBlocks = true;

        auxpowConsensus = digishieldConsensus;
        auxpowConsensus.nHeightEffective = std::numeric_limits<uint32_t>::max();

        pConsensusRoot = &digishieldConsensus;
        digishieldConsensus.pLeft = &consensus;
        digishieldConsensus.pRight = &auxpowConsensus;
    }

    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int64_t nStartHeight, int64_t nTimeoutHeight)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].nStartHeight = nStartHeight;
        consensus.vDeployments[d].nTimeoutHeight = nTimeoutHeight;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range.", height));
        } else if (height == -1) {
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() < 3 || 5 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:heightstart:heightend]");
        }
        int64_t nStartTime, nTimeout, nStartHeight, nTimeoutHeight;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() > 3 && !ParseInt64(vDeploymentParams[3], &nStartHeight)) {
            throw std::runtime_error(strprintf("Invalid nStartHeight (%s)", vDeploymentParams[3]));
        }
        if (vDeploymentParams.size() > 4 && !ParseInt64(vDeploymentParams[4], &nTimeoutHeight)) {
            throw std::runtime_error(strprintf("Invalid nTimeoutHeight (%s)", vDeploymentParams[4]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, nStartHeight, nTimeoutHeight);
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}

const Consensus::Params *Consensus::Params::GetConsensus(uint32_t nTargetHeight) const {
    if (nTargetHeight < this->nHeightEffective && this->pLeft != nullptr) {
        return this->pLeft->GetConsensus(nTargetHeight);
    } else if (nTargetHeight > this->nHeightEffective && this->pRight != nullptr) {
        const Consensus::Params *pCandidate = this->pRight->GetConsensus(nTargetHeight);
        if (pCandidate->nHeightEffective <= nTargetHeight) {
            return pCandidate;
        }
    }
    return this;
}
