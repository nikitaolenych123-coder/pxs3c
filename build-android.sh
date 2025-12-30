#!/bin/bash
# Build script for PXS3C Android APK
# Prerequisites: Android SDK, NDK, Java 11+

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}PXS3C Android Build Script${NC}"
echo "============================="

# Check prerequisites
check_requirement() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}✗ $1 not found${NC}"
        return 1
    else
        echo -e "${GREEN}✓ $1 found${NC}"
        return 0
    fi
}

echo ""
echo -e "${YELLOW}Checking prerequisites:${NC}"
check_requirement "java" || exit 1
check_requirement "gradle" || exit 1

# Check Android SDK
if [ -z "$ANDROID_HOME" ]; then
    echo -e "${RED}✗ ANDROID_HOME not set${NC}"
    exit 1
else
    echo -e "${GREEN}✓ ANDROID_HOME=${ANDROID_HOME}${NC}"
fi

# Build
echo ""
echo -e "${YELLOW}Building Android APK...${NC}"
cd "$(dirname "$0")/android"

echo "Step 1: Clean previous builds..."
gradle clean

echo ""
echo "Step 2: Build debug APK..."
gradle assembleDebug

if [ -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
    APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
    echo ""
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo -e "${GREEN}APK: ${APK_PATH}${NC}"
    echo ""
    echo -e "${YELLOW}Installation instructions:${NC}"
    echo "1. Connect Android device via USB"
    echo "2. Enable Developer Mode and USB Debugging"
    echo "3. Run: adb install -r \"${APK_PATH}\""
    echo ""
    echo "Or build release APK with:"
    echo "gradle assembleRelease"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi
