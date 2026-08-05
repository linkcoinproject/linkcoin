#ifndef BITCOIN_CHAINPARAMSSEEDS_H
#define BITCOIN_CHAINPARAMSSEEDS_H
/**
 * List of fixed seed nodes for the Linkcoin network
 *
 * Linkcoin P2P port: 7200 (0x1C20)
 *
 * Each line is a BIP155 serialized (networkID, addr, port) tuple.
 * Format: 0x01 (IPv4), 4 bytes addr, 2 bytes port (big endian)
 */
static const uint8_t chainparams_seed_main[] = {
    // 103.133.25.201:7200
    0x01, 0x04, 0x67, 0x85, 0x19, 0xc9, 0x1c, 0x20,
    // 159.223.90.59:7200
    0x01, 0x04, 0x9f, 0xdf, 0x5a, 0x3b, 0x1c, 0x20,
};

static const uint8_t chainparams_seed_test[] = {
    // No hardcoded testnet seeds (port 72555 > 16-bit range)
};

static const uint8_t chainparams_seed_regtest[] = {
    // No regtest seeds
};
#endif // BITCOIN_CHAINPARAMSSEEDS_H
