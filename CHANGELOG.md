# Linkcoin Changelog

## Version 0.9.1.0 - Classic2 Adaptive Difficulty (2025-11-24)

### Major Changes

#### Classic2 Adaptive Difficulty Algorithm (Block 2500+)
- **Activation Height**: Block 2500
- **Averaging Window**: 24 blocks (96 minutes)
- **Max Adjustment**: 16% down, 8% up per adjustment
- **Dampening Factor**: 3 for smoother transitions
- **Safety Limit**: Maximum 50% difficulty drop per adjustment
- **Timestamp Validation**: Enhanced protection against time-warp attacks

#### Key Improvements
- **Prevents Rapid Difficulty Drops**: Tighter bounds (16% vs 50% in Legacy DigiShield)
- **Better Stability**: Dampening factor reduces volatility
- **Attack Resistance**: Multiple safety mechanisms prevent manipulation
- **Responsive**: 24-block window balances stability and responsiveness

### Network Changes
- Added seed node: `159.223.90.59`
- Improved DNS seed configuration for better peer discovery

### Algorithm Timeline
```
Block 0-387:    Original Algorithm (interval-based retargeting)
Block 388-2499: Legacy DigiShield (60-block average, asymmetric dampening)
Block 2500+:    Classic2 Adaptive (24-block average, symmetric dampening)
```

### Technical Details

**Classic2 Parameters:**
- `nClassic2Height = 2500`
- `nClassic2AveragingWindow = 24`
- `nClassic2MaxAdjustDown = 16%`
- `nClassic2MaxAdjustUp = 8%`
- `nClassic2DampeningFactor = 3`

**Safety Features:**
1. Dampening: `nAdjustedTimespan = nTargetTimespan + (nActualTimespan - nTargetTimespan) / 3`
2. Bounds: Min 92% / Max 116% of target timespan
3. Hard limit: Maximum 50% drop per single adjustment
4. Timestamp validation: Reject blocks with suspicious timestamps

### Compatibility
- Fully backward compatible with existing blockchain
- All mining software supporting `getblocktemplate` will automatically use new difficulty
- No configuration changes required for miners or nodes

### Documentation
- Added `DIFFICULTY_ALGORITHM.md` with complete algorithm documentation
- Detailed comparison between Legacy DigiShield and Classic2

---

## Version 0.9.0.0 - Previous Release

### Features
- Legacy DigiShield implementation (Block 388+)
- Basic network infrastructure
- Standard Bitcoin-based functionality
