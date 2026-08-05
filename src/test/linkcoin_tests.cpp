// Copyright (c) 2014-2025 The Linkcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <junkcoin.h>
#include <primitives/block.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(linkcoin_tests, TestingSetup)

uint64_t expectedMaxSubsidy(int height) {
    int64_t nSubsidy = 80 * COIN;
    if (height == 10) return 500000 * COIN;
    nSubsidy >>= (height / 400000);
    return nSubsidy;
}

BOOST_AUTO_TEST_CASE(subsidy_test)
{
    const auto& params = Params().GetConsensus();

    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(0, params), 80 * COIN);
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(1, params), 80 * COIN);
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(10, params), 500000 * COIN);
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(11, params), 80 * COIN);
    // First halving at 400k
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(400000, params), 40 * COIN);
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(400001, params), 40 * COIN);
    // Second halving at 800k
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(800000, params), 20 * COIN);
    // Third halving at 1.2M
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(1200000, params), 10 * COIN);
    // Fourth halving at 1.6M
    BOOST_CHECK_EQUAL(GetLinkcoinBlockSubsidy(1600000, params), 5 * COIN);
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto& params = Params().GetConsensus();

    for (int h = 0; h < 400000; h += 50000) {
        CAmount nSubsidy = GetLinkcoinBlockSubsidy(h, params);
        CAmount maxSubsidy = expectedMaxSubsidy(h);
        if (h != 10) {
            BOOST_CHECK(nSubsidy <= maxSubsidy);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
