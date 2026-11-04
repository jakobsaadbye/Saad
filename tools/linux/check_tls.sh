#!/usr/bin/env bash
# Checks https of net.http (TLS with OpenSSL) against a local 'openssl s_server' with a certificate of its own:
# a request when the certificate is trusted, and an error when it isn't, or when it is of another host.
#
#   tools/linux/check_tls.sh
#
# Requirements: openssl, gcc, libssl-dev. Build the compiler first with ./build.sh
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
TMP="$(mktemp -d)"
SERVER=""
trap '[ -n "$SERVER" ] && kill $SERVER 2>/dev/null; rm -rf "$TMP"' EXIT

"$REPO/bin/saad" build "$REPO/tests/tls/https_get.sd" --elf > /dev/null || exit 1
bash "$HERE/link.sh" "$REPO/tests/tls/build/out.o" "$TMP/https_get" || exit 1

openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost" \
    -keyout "$TMP/key.pem" -out "$TMP/cert.pem" > /dev/null 2>&1 || { echo "openssl couldn't make a certificate"; exit 1; }
PORT=$((40000 + RANDOM % 10000))
openssl s_server -accept "$PORT" -cert "$TMP/cert.pem" -key "$TMP/key.pem" -www -quiet > /dev/null 2>&1 &
SERVER=$!
for _ in $(seq 50); do
    (echo > /dev/tcp/127.0.0.1/$PORT) 2> /dev/null && break
    sleep 0.1
done

failed=0
expect() {   # expect <what> <text the output must have> <command...>
    local output
    output=$(timeout 20 "${@:3}" 2>&1)
    if ! printf '%s' "$output" | grep -qF -- "$2"; then
        echo "FAILED: $1"
        echo "$output" | head -5 | sed 's/^/    /'
        failed=1
    fi
}

expect "a trusted certificate" "status 200" env SSL_CERT_FILE="$TMP/cert.pem" "$TMP/https_get" "https://localhost:$PORT/"
expect "the page of s_server" "s_server" env SSL_CERT_FILE="$TMP/cert.pem" "$TMP/https_get" "https://localhost:$PORT/"
expect "an unknown certificate" "net.TlsError" env SSL_CERT_FILE=/nonexistent SSL_CERT_DIR=/nonexistent "$TMP/https_get" "https://localhost:$PORT/"
expect "a certificate of another host" "net.TlsError" env SSL_CERT_FILE="$TMP/cert.pem" "$TMP/https_get" "https://127.0.0.1:$PORT/"
exit $failed
