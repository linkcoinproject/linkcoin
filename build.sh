#!/bin/bash
# Simple build script for LinkCoin using depends system
# Usage: ./build.sh [--linux|--windows] [--daemon-only]

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
PROJECT_DIR=$(cd "$(dirname "$0")" && pwd)
# Use single core by default to avoid memory issues during compilation
NUM_CORES=1
PLATFORM=""
BUILD_DAEMON=1
BUILD_QT=0
CLEAN_ONLY=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --linux)
            PLATFORM="linux"
            shift
            ;;
        --windows)
            PLATFORM="windows"
            shift
            ;;
        --daemon-only)
            BUILD_DAEMON=1
            BUILD_QT=0
            shift
            ;;
        --qt-only)
            BUILD_DAEMON=0
            BUILD_QT=1
            shift
            ;;
        --both)
            BUILD_DAEMON=1
            BUILD_QT=1
            shift
            ;;
        -j|--jobs)
            NUM_CORES="$2"
            shift 2
            ;;
        --clean)
            CLEAN_ONLY=1
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --linux          Build for Linux"
            echo "  --windows        Build for Windows"
            echo "  --daemon-only    Build only daemon (default)"
            echo "  --qt-only        Build only Qt GUI"
            echo "  --both           Build both daemon and Qt"
            echo "  -j, --jobs N     Use N parallel jobs (default: 1 to avoid memory issues)"
            echo "  --clean          Clean build artifacts"
            echo "  --help           Show this help"
            echo ""
            echo "Prerequisites:"
            echo "  1. Build depends first (uses Qt 4.6 by default):"
            echo "     cd depends && make HOST=x86_64-pc-linux-gnu"
            echo "     cd depends && make HOST=x86_64-w64-mingw32"
            echo ""
            echo "Examples:"
            echo "  $0 --linux --daemon-only"
            echo "  $0 --windows --daemon-only"
            echo "  $0 --linux --daemon-only -j 2"
            echo "  $0 --linux --clean"
            echo "  $0 --windows --clean"
            echo ""
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage"
            exit 1
            ;;
    esac
done

# Auto-detect platform if not specified
if [ -z "$PLATFORM" ]; then
    echo -e "${YELLOW}No platform specified, checking available depends...${NC}"
    
    if [ -d "$PROJECT_DIR/depends/x86_64-pc-linux-gnu" ]; then
        PLATFORM="linux"
        echo -e "${GREEN}Found Linux depends, building for Linux${NC}"
    elif [ -d "$PROJECT_DIR/depends/x86_64-w64-mingw32" ]; then
        PLATFORM="windows"
        echo -e "${GREEN}Found Windows depends, building for Windows${NC}"
    else
        echo -e "${RED}Error: No depends found!${NC}"
        echo ""
        echo "Please build depends first:"
        echo "  cd depends && make HOST=x86_64-pc-linux-gnu"
        echo "  or"
        echo "  cd depends && make HOST=x86_64-w64-mingw32"
        echo ""
        echo "Note: This will use Qt 4.6 by default (compatible with LinkCoin)"
        exit 1
    fi
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}LinkCoin Build Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Configuration:"
echo "  Platform: $PLATFORM"
echo "  Build Daemon: $BUILD_DAEMON"
echo "  Build Qt: $BUILD_QT"
echo "  CPU Cores: $NUM_CORES"
echo ""

# Clean function for Linux
clean_linux() {
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Cleaning Linux build artifacts${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
    echo "Cleaning daemon build..."
    make -f Makefile.depends.linux clean 2>/dev/null || true
    
    echo "Cleaning Qt build..."
    if [ -f "Makefile" ]; then
        make clean 2>/dev/null || true
        rm -f Makefile
    fi
    
    echo "Removing binaries..."
    rm -f src/linkcoind
    rm -f linkcoin-qt
    
    echo "Removing object files..."
    rm -rf src/obj/*.o
    rm -rf src/obj/*.d
    
    echo ""
    echo -e "${GREEN}✓ Linux build artifacts cleaned${NC}"
    echo ""
}

# Clean function for Windows
clean_windows() {
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Cleaning Windows build artifacts${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
    echo "Cleaning daemon build..."
    make -f Makefile.depends.windows clean 2>/dev/null || true
    
    echo "Cleaning Qt build..."
    if [ -f "Makefile" ]; then
        make clean 2>/dev/null || true
        rm -f Makefile
    fi
    
    echo "Removing binaries..."
    rm -f src/linkcoind.exe
    rm -rf release/
    rm -rf debug/
    
    echo "Removing object files..."
    rm -rf src/obj/*.o
    rm -rf src/obj/*.d
    
    echo ""
    echo -e "${GREEN}✓ Windows build artifacts cleaned${NC}"
    echo ""
}

# Build daemon
build_daemon() {
    local makefile=$1
    local binary=$2
    
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Building linkcoind${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
    # Fix leveldb permissions
    chmod +x src/leveldb/build_detect_platform 2>/dev/null || true
    
    # Build (no clean - incremental build like direct make command)
    echo "Building with $makefile..."
    make -f "$makefile" -j$NUM_CORES
    
    echo ""
    if [ -f "$binary" ]; then
        echo -e "${GREEN}✓ Build successful: $binary${NC}"
        ls -lh "$binary"
        
        # Show file info
        file "$binary"
        
        # Check linking (Linux only)
        if [[ "$PLATFORM" == "linux" ]]; then
            echo ""
            echo "Linked libraries:"
            ldd "$binary" 2>&1 | head -20 || echo "Fully static binary"
        fi
    else
        echo -e "${RED}✗ Build failed: $binary not found${NC}"
        exit 1
    fi
    
    echo ""
}

# Build Qt GUI
build_qt() {
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Building linkcoin-qt${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    cd "$PROJECT_DIR"
    
    # Determine depends prefix and qmake
    local DEPENDS_PREFIX=""
    local QMAKE_BIN="qmake"
    
    if [ "$PLATFORM" = "linux" ]; then
        DEPENDS_PREFIX="$PROJECT_DIR/depends/x86_64-pc-linux-gnu"
        if [ -f "$DEPENDS_PREFIX/bin/qmake" ]; then
            QMAKE_BIN="$DEPENDS_PREFIX/bin/qmake"
        else
            echo -e "${RED}Error: Qt not found in depends${NC}"
            echo "Build Qt first: cd depends && make HOST=x86_64-pc-linux-gnu"
            return 1
        fi
    else
        DEPENDS_PREFIX="$PROJECT_DIR/depends/x86_64-w64-mingw32"
        if [ -f "$DEPENDS_PREFIX/bin/qmake" ]; then
            QMAKE_BIN="$DEPENDS_PREFIX/bin/qmake"
        else
            echo -e "${RED}Error: Qt not found in depends${NC}"
            echo "Build Qt first: cd depends && make HOST=x86_64-w64-mingw32"
            return 1
        fi
    fi
    
    # Configure (skip clean for incremental builds)
    echo "Configuring with $QMAKE_BIN..."
    
    local BOOST_SUFFIX=""
    if [ "$PLATFORM" = "windows" ]; then
        BOOST_SUFFIX="-mt-s"
        export QMAKESPEC="$DEPENDS_PREFIX/mkspecs/win32-g++"
        export QMAKE_RANLIB="x86_64-w64-mingw32-ranlib"
    else
        BOOST_SUFFIX="-mt"
        export QMAKESPEC="$DEPENDS_PREFIX/mkspecs/linux-g++-64"
    fi
    
    $QMAKE_BIN \
        "RELEASE=1" \
        "USE_UPNP=1" \
        "USE_QRCODE=1" \
        BOOST_INCLUDE_PATH="$DEPENDS_PREFIX/include" \
        BOOST_LIB_PATH="$DEPENDS_PREFIX/lib" \
        BOOST_LIB_SUFFIX="$BOOST_SUFFIX" \
        BDB_INCLUDE_PATH="$DEPENDS_PREFIX/include" \
        BDB_LIB_PATH="$DEPENDS_PREFIX/lib" \
        OPENSSL_INCLUDE_PATH="$DEPENDS_PREFIX/include" \
        OPENSSL_LIB_PATH="$DEPENDS_PREFIX/lib" \
        MINIUPNPC_INCLUDE_PATH="$DEPENDS_PREFIX/include" \
        MINIUPNPC_LIB_PATH="$DEPENDS_PREFIX/lib" \
        QRENCODE_INCLUDE_PATH="$DEPENDS_PREFIX/include" \
        QRENCODE_LIB_PATH="$DEPENDS_PREFIX/lib" \
        QMAKE_RANLIB="$QMAKE_RANLIB" \
        bitcoin-qt.pro
    
    # Build
    echo "Building..."
    
    # Fix GDI32 linking issue: add -lgdi32 immediately after -lcrypto in the Makefile
    if [ "$PLATFORM" = "windows" ]; then
        echo "Patching Makefile to fix GDI32 link order..."
        sed -i 's/-lcrypto /-lcrypto -lgdi32 /g' Makefile.Release
    fi
    
    make -j$NUM_CORES
    
    echo ""
    if [ "$PLATFORM" = "linux" ] && [ -f "linkcoin-qt" ]; then
        echo -e "${GREEN}✓ Build successful: linkcoin-qt${NC}"
        ls -lh linkcoin-qt
    elif [ "$PLATFORM" = "windows" ] && [ -f "release/linkcoin-qt.exe" ]; then
        echo -e "${GREEN}✓ Build successful: release/linkcoin-qt.exe${NC}"
        ls -lh release/linkcoin-qt.exe
    else
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    fi
    
    echo ""
}

# Main build logic
main() {
    local start_time=$(date +%s)
    
    if [ "$PLATFORM" = "linux" ]; then
        # Handle clean
        if [ $CLEAN_ONLY -eq 1 ]; then
            clean_linux
            exit 0
        fi
        
        # Check depends
        if [ ! -d "$PROJECT_DIR/depends/x86_64-pc-linux-gnu" ]; then
            echo -e "${RED}Error: Linux depends not found${NC}"
            echo "Build depends first: cd depends && make HOST=x86_64-pc-linux-gnu"
            exit 1
        fi
        
        # Build daemon
        if [ $BUILD_DAEMON -eq 1 ]; then
            build_daemon "Makefile.depends.linux" "src/linkcoind"
        fi
        
        # Build Qt
        if [ $BUILD_QT -eq 1 ]; then
            build_qt
        fi
        
    elif [ "$PLATFORM" = "windows" ]; then
        # Handle clean
        if [ $CLEAN_ONLY -eq 1 ]; then
            clean_windows
            exit 0
        fi
        
        # Check depends
        if [ ! -d "$PROJECT_DIR/depends/x86_64-w64-mingw32" ]; then
            echo -e "${RED}Error: Windows depends not found${NC}"
            echo "Build depends first: cd depends && make HOST=x86_64-w64-mingw32"
            exit 1
        fi
        
        # Check MinGW
        if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
            echo -e "${RED}Error: MinGW not found${NC}"
            echo "Install: sudo apt-get install g++-mingw-w64-x86-64"
            exit 1
        fi
        
        # Build daemon
        if [ $BUILD_DAEMON -eq 1 ]; then
            build_daemon "Makefile.depends.windows" "src/linkcoind.exe"
        fi
        
        # Build Qt
        if [ $BUILD_QT -eq 1 ]; then
            build_qt
        fi
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Build Complete!${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    echo "Build time: $((duration / 60)) minutes $((duration % 60)) seconds"
    echo ""
    
    # Show results
    echo "Binaries:"
    if [ "$PLATFORM" = "linux" ]; then
        [ -f "src/linkcoind" ] && echo -e "${GREEN}  ✓ src/linkcoind${NC}"
        [ -f "linkcoin-qt" ] && echo -e "${GREEN}  ✓ linkcoin-qt${NC}"
    else
        [ -f "src/linkcoind.exe" ] && echo -e "${GREEN}  ✓ src/linkcoind.exe${NC}"
        [ -f "release/linkcoin-qt.exe" ] && echo -e "${GREEN}  ✓ release/linkcoin-qt.exe${NC}"
    fi
    echo ""
}

main
