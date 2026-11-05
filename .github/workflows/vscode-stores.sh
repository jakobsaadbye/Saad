# If the stores give a version of the VS Code extension to everyone. Sourced by vscode-extension.yml, with EXTENSION set,
# e.g jakobsaadbye.saad. Requirements: curl and jq

# If the Visual Studio Marketplace has the version, verified. A version that is published but not verified yet isn't
# given to anyone, and isn't in the answer: the flags are IncludeVersions (0x1) and ExcludeNonValidated (0x20)
marketplace_has() {
    curl -fsS --retry 3 -X POST "https://marketplace.visualstudio.com/_apis/public/gallery/extensionquery" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json;api-version=3.0-preview.1" \
        -d "{\"filters\":[{\"criteria\":[{\"filterType\":7,\"value\":\"$EXTENSION\"}]}],\"flags\":33}" \
        | jq -r '.results[0].extensions[0].versions[]?.version' | grep -qxF "$1"
}

# If Open VSX gives the version. It is 404 until it is published and active
openvsx_has() {
    status="$(curl -sS --retry 3 -o /dev/null -w '%{http_code}' "https://open-vsx.org/api/${EXTENSION/./\/}/$1")"
    [ "$status" = 200 ]
}
