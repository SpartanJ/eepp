#!/bin/bash
set -eo pipefail

# Change to the script's directory to ensure relative paths are stable.
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit

# This script handles code signing and notarization for the macOS app.
# It's designed to be called from the CI environment.
#
# It expects the following environment variables to be set for real signing:
# - MACOS_CERTIFICATE_P12_B64: The base64 encoded .p12 certificate.
# - MACOS_CERTIFICATE_PASSWORD: The password for the .p12 certificate.
# - MACOS_APPLE_ID: Your Apple ID email used for notarization.
# - MACOS_NOTARIZATION_PASSWORD: An app-specific password for your Apple ID.
# - MACOS_TEAM_ID: Your Apple Developer Team ID.
#
# If these variables are not set, it will fall back to ad-hoc (self) signing for .app bundles
# and skip notarization for .dmg files.

# The first argument is the path to the artifact, relative to this script's location.
ARTIFACT_PATH="$1"
# The entitlements file is in the same directory as this script.
ENTITLEMENTS_PATH="entitlements.plist"

if [[ -z "$ARTIFACT_PATH" ]]; then
    echo "Usage: $0 <path_to_app_or_dmg>"
    exit 1
fi

if [[ ! -e "$ARTIFACT_PATH" ]]; then
    echo "Error: Artifact not found at '$ARTIFACT_PATH'"
    exit 1
fi

# --- AD-HOC SIGNING (if no credentials) ---
if [[ -z "$MACOS_CERTIFICATE_P12_B64" ]]; then
    if [[ "$ARTIFACT_PATH" == *.app ]]; then
        echo "No signing certificate found. Performing ad-hoc signing..."
        # Find and sign all binaries within the app bundle
        find "$ARTIFACT_PATH/Contents/MacOS/" -type f -exec codesign --force --sign - {} \;
        codesign --force --sign - "$ARTIFACT_PATH"
        echo "Ad-hoc signing complete."
    fi
    # For .dmg files, we just skip if no credentials
    exit 0
fi

echo "Credentials found. Proceeding with official signing and notarization..."

# --- KEYCHAIN AND CERTIFICATE SETUP ---
KEYCHAIN_NAME="build.keychain"
KEYCHAIN_PASSWORD="a-very-secure-password"
CERTIFICATE_P12_PATH="certificate.p12"

# Decode the certificate
echo "$MACOS_CERTIFICATE_P12_B64" | base64 --decode > "$CERTIFICATE_P12_PATH"

# Create a temporary keychain
security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME"
security default-keychain -s "$KEYCHAIN_NAME"
security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME"
security set-keychain-settings -t 3600 -u "$KEYCHAIN_NAME"

# Import the certificate into the keychain
security import "$CERTIFICATE_P12_PATH" -k "$KEYCHAIN_NAME" -P "$MACOS_CERTIFICATE_PASSWORD" -T /usr/bin/codesign -T /usr/bin/security

# Allow codesign to access the certificate
security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME" > /dev/null

# Find the signing identity
SIGNING_IDENTITY=$(security find-identity -v -p codesigning "$KEYCHAIN_NAME" | grep "Developer ID Application" | head -n 1 | awk -F '"' '{print $2}')
if [[ -z "$SIGNING_IDENTITY" ]]; then
    echo "Error: Signing identity not found in keychain."
    exit 1
fi
echo "Using signing identity: $SIGNING_IDENTITY"


# --- MAIN LOGIC ---

# Function to sign the .app bundle
sign_app() {
    echo "Signing application bundle at: $ARTIFACT_PATH"
    # Sign all dylibs, frameworks and executables from the inside out
    find "$ARTIFACT_PATH" -depth -name "*.dylib" -o -name "*.framework" -o -path "$ARTIFACT_PATH/Contents/MacOS/*" -type f | while read -r comp; do
        echo "Signing component: $comp"
        codesign --force --verify --verbose --sign "$SIGNING_IDENTITY" --options runtime --timestamp "$comp"
    done

    echo "Signing main application bundle with entitlements..."
    codesign --force --verify --verbose --sign "$SIGNING_IDENTITY" --entitlements "$ENTITLEMENTS_PATH" --options runtime --timestamp "$ARTIFACT_PATH"
    echo "App signing complete."
}

# Function to notarize and staple the .dmg
notarize_dmg() {
    echo "Notarizing DMG at: $ARTIFACT_PATH"

    # Temporary file to store the command's output
    local notary_output_file
    notary_output_file=$(mktemp)

    # Submit for notarization and check the exit code directly.
    # The --wait flag makes the command exit with 0 on success and non-zero on failure.
    # We redirect all output to a temp file so we can show it and parse it later.
    if ! xcrun notarytool submit "$ARTIFACT_PATH" \
        --apple-id "$MACOS_APPLE_ID" \
        --password "$MACOS_NOTARIZATION_PASSWORD" \
        --team-id "$MACOS_TEAM_ID" \
        --wait > "$notary_output_file" 2>&1; then

        echo "Error: Notarization failed."
        # Print the full output from the failed command for debugging
        cat "$notary_output_file"

        # Attempt to get logs if we can find a UUID.
        # Use 'head -n 1' to ensure we only get the first matching line.
        REQUEST_UUID=$(grep "id:" "$notary_output_file" | head -n 1 | awk '{print $2}')
        if [[ -n "$REQUEST_UUID" ]]; then
            echo "Fetching notarization logs for UUID: $REQUEST_UUID"
            xcrun notarytool log "$REQUEST_UUID" \
                --apple-id "$MACOS_APPLE_ID" \
                --password "$MACOS_NOTARIZATION_PASSWORD" \
                --team-id "$MACOS_TEAM_ID"
        fi
        rm "$notary_output_file"
        exit 1
    fi

    # If we reach here, the command succeeded.
    echo "Notarization successful. Full log:"
    cat "$notary_output_file"
    rm "$notary_output_file"

    echo "Stapling ticket to DMG..."
    xcrun stapler staple "$ARTIFACT_PATH"
    echo "Stapling complete."
}

# --- CLEANUP ---
cleanup() {
    echo "Cleaning up..."
    security delete-keychain "$KEYCHAIN_NAME" || true
    rm -f "$CERTIFICATE_P12_PATH"
}
trap cleanup EXIT

# --- EXECUTION ---
if [[ "$ARTIFACT_PATH" == *.app ]]; then
    sign_app
elif [[ "$ARTIFACT_PATH" == *.dmg ]]; then
    notarize_dmg
else
    echo "Unsupported artifact type: $ARTIFACT_PATH"
    exit 1
fi
