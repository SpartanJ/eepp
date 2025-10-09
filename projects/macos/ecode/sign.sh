#!/bin/bash
set -eo pipefail

# Change to the script's directory to ensure relative paths are stable.
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit

# This script handles code signing and notarization for the macOS app.
# It expects the following environment variables to be set for real signing:
# - MACOS_CERTIFICATE_P12_B64: The base64 encoded .p12 certificate.
# - MACOS_CERTIFICATE_PASSWORD: The password for the .p12 certificate.
# - MACOS_APPLE_ID: Your Apple ID email used for notarization.
# - MACOS_NOTARIZATION_PASSWORD: An app-specific password for your Apple ID.
# - MACOS_TEAM_ID: Your Apple Developer Team ID.

# The first argument is the path to the artifact, relative to this script's location.
ARTIFACT_PATH="$1"
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
        find "$ARTIFACT_PATH/Contents/MacOS/" -type f -exec codesign --force --sign - {} \;
        codesign --force --sign - "$ARTIFACT_PATH"
        echo "Ad-hoc signing complete."
    fi
    exit 0
fi

echo "Credentials found. Proceeding with official signing and notarization..."

# --- KEYCHAIN AND CERTIFICATE SETUP ---
KEYCHAIN_NAME="build.keychain"
KEYCHAIN_PASSWORD="a-very-secure-password"
CERTIFICATE_P12_PATH="certificate.p12"

# Check if keychain already exists
if [[ -f "$HOME/Library/Keychains/$KEYCHAIN_NAME-db" ]]; then
    echo "Keychain $KEYCHAIN_NAME already exists. Reusing it..."
    security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME"
else
    echo "Creating temporary keychain: $KEYCHAIN_NAME"
    security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME"
    security default-keychain -s "$KEYCHAIN_NAME"
    security set-keychain-settings -t 3600 -u "$KEYCHAIN_NAME"

    # Decode and import the certificate
    echo "Decoding certificate..."
    echo "$MACOS_CERTIFICATE_P12_B64" | base64 --decode > "$CERTIFICATE_P12_PATH"
    if [[ ! -s "$CERTIFICATE_P12_PATH" ]]; then
        echo "Error: Certificate file is empty or invalid."
        exit 1
    fi

    echo "Importing certificate into keychain..."
    security import "$CERTIFICATE_P12_PATH" -k "$KEYCHAIN_NAME" -P "$MACOS_CERTIFICATE_PASSWORD" -T /usr/bin/codesign -T /usr/bin/security
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to import certificate."
        exit 1
    fi

    # Allow codesign access
    echo "Setting keychain partition list..."
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME" > /dev/null
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to set keychain partition list."
        exit 1
    fi
fi

# Ensure keychain is unlocked
echo "Ensuring keychain is unlocked..."
security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME"

# Find the signing identity
echo "Searching for signing identity..."
SIGNING_IDENTITY=$(security find-identity -v -p codesigning "$KEYCHAIN_NAME" | grep "Developer ID Application" | head -n 1 | awk -F '"' '{print $2}')
if [[ -z "$SIGNING_IDENTITY" ]]; then
    echo "Error: No Developer ID Application signing identity found."
    security find-identity -v -p codesigning "$KEYCHAIN_NAME"
    exit 1
fi
echo "Using signing identity: $SIGNING_IDENTITY"

# --- MAIN LOGIC ---

# Function to sign the .app bundle
sign_app() {
    echo "Signing application bundle at: $ARTIFACT_PATH"
    find "$ARTIFACT_PATH" -depth -name "*.dylib" -o -name "*.framework" -o -path "$ARTIFACT_PATH/Contents/MacOS/*" -type f | while read -r comp; do
        echo "Signing component: $comp"
        codesign --force --verbose --sign "$SIGNING_IDENTITY" --options runtime --timestamp "$comp"
    done

    echo "Signing main application bundle with entitlements..."
    codesign --force --verbose --sign "$SIGNING_IDENTITY" --entitlements "$ENTITLEMENTS_PATH" --options runtime --timestamp "$ARTIFACT_PATH"
    echo "App signing complete."

    # Notarize the app
    ZIP_PATH="${ARTIFACT_PATH%.*}.zip"
    echo "Zipping app for notarization: $ZIP_PATH"
    ditto -c -k --sequesterRsrc --keepParent "$ARTIFACT_PATH" "$ZIP_PATH"

    local notary_output_file
    notary_output_file=$(mktemp)

    echo "Notarizing app zip..."
    if ! xcrun notarytool submit "$ZIP_PATH" \
        --apple-id "$MACOS_APPLE_ID" \
        --password "$MACOS_NOTARIZATION_PASSWORD" \
        --team-id "$MACOS_TEAM_ID" \
        --wait > "$notary_output_file" 2>&1; then

        echo "Error: App notarization failed."
        cat "$notary_output_file"
        REQUEST_UUID=$(grep "id:" "$notary_output_file" | head -n 1 | awk '{print $2}')
        if [[ -n "$REQUEST_UUID" ]]; then
            echo "Fetching notarization logs for UUID: $REQUEST_UUID"
            xcrun notarytool log "$REQUEST_UUID" \
                --apple-id "$MACOS_APPLE_ID" \
                --password "$MACOS_NOTARIZATION_PASSWORD" \
                --team-id "$MACOS_TEAM_ID"
        fi
        rm "$notary_output_file"
        rm -f "$ZIP_PATH"
        exit 1
    fi

    echo "App notarization successful. Full log:"
    cat "$notary_output_file"
    rm "$notary_output_file"

    echo "Stapling ticket to app..."
    xcrun stapler staple "$ARTIFACT_PATH"
    echo "App stapling complete."

    rm -f "$ZIP_PATH"
}

# Function to notarize and staple the .dmg
notarize_dmg() {
    echo "Notarizing DMG at: $ARTIFACT_PATH"

    # Re-verify keychain state
    echo "Verifying keychain state before DMG signing..."
    security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN_NAME"
    security find-identity -v -p codesigning "$KEYCHAIN_NAME"

    # Sign the DMG
    echo "Signing DMG..."
    codesign --force --verbose --sign "$SIGNING_IDENTITY" "$ARTIFACT_PATH"
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to sign DMG. Checking keychain state..."
        security find-identity -v -p codesigning "$KEYCHAIN_NAME"
        exit 1
    fi
    echo "DMG signing complete."

    # Verify the signature
    echo "Verifying DMG signature..."
    codesign --verify --verbose "$ARTIFACT_PATH"

    # Notarize the DMG
    local notary_output_file
    notary_output_file=$(mktemp)

    echo "Notarizing DMG..."
    if ! xcrun notarytool submit "$ARTIFACT_PATH" \
        --apple-id "$MACOS_APPLE_ID" \
        --password "$MACOS_NOTARIZATION_PASSWORD" \
        --team-id "$MACOS_TEAM_ID" \
        --wait > "$notary_output_file" 2>&1; then

        echo "Error: Notarization failed."
        cat "$notary_output_file"
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

    echo "Notarization successful. Full log:"
    cat "$notary_output_file"
    rm "$notary_output_file"

    echo "Stapling ticket to DMG..."
    xcrun stapler staple "$ARTIFACT_PATH"
    echo "Stapling complete."
}

# --- CLEANUP ---
# Note: Cleanup is handled in CI, not here, since this script is called twice
# cleanup() {
#     echo "Cleaning up..."
#     security delete-keychain "$KEYCHAIN_NAME" || true
#     rm -f "$CERTIFICATE_P12_PATH"
# }
# trap cleanup EXIT

# --- EXECUTION ---
if [[ "$ARTIFACT_PATH" == *.app ]]; then
    sign_app
elif [[ "$ARTIFACT_PATH" == *.dmg ]]; then
    notarize_dmg
else
    echo "Unsupported artifact type: $ARTIFACT_PATH"
    exit 1
fi
