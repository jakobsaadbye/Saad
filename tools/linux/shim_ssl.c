//////////////////////////////////////////////////////////////////////////////////////////
// Linux test harness: the OpenSSL functions of std/net/tls.sd as ms_abi wrappers, like shim.c.
// link.sh links this (and -lssl -lcrypto) only into programs that call them. Needs libssl-dev
//////////////////////////////////////////////////////////////////////////////////////////
#define MS __attribute__((ms_abi))
#include <stdint.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>

MS const SSL_METHOD *ms_TLS_client_method(void) { return TLS_client_method(); }
MS SSL_CTX *ms_SSL_CTX_new(const SSL_METHOD *m) { return SSL_CTX_new(m); }
MS void     ms_SSL_CTX_free(SSL_CTX *c) { SSL_CTX_free(c); }
MS int      ms_SSL_CTX_set_default_verify_paths(SSL_CTX *c) { return SSL_CTX_set_default_verify_paths(c); }
MS void     ms_SSL_CTX_set_verify(SSL_CTX *c, int mode, void *callback) { (void) callback; SSL_CTX_set_verify(c, mode, NULL); }
MS SSL     *ms_SSL_new(SSL_CTX *c) { return SSL_new(c); }
MS void     ms_SSL_free(SSL *s) { SSL_free(s); }
MS int      ms_SSL_set_fd(SSL *s, int fd) { return SSL_set_fd(s, fd); }
MS long     ms_SSL_ctrl(SSL *s, int command, long larg, void *parg) { return SSL_ctrl(s, command, larg, parg); }
MS int      ms_SSL_set1_host(SSL *s, const char *host) { return SSL_set1_host(s, host); }
MS int      ms_SSL_connect(SSL *s) { return SSL_connect(s); }
MS int      ms_SSL_read(SSL *s, void *buffer, int count) { return SSL_read(s, buffer, count); }
MS int      ms_SSL_write(SSL *s, const void *buffer, int count) { return SSL_write(s, buffer, count); }
MS int      ms_SSL_get_error(const SSL *s, int result) { return SSL_get_error(s, result); }
MS int      ms_SSL_shutdown(SSL *s) { return SSL_shutdown(s); }
MS long     ms_SSL_get_verify_result(const SSL *s) { return SSL_get_verify_result(s); }
MS unsigned long ms_ERR_get_error(void) { return ERR_get_error(); }
MS void     ms_ERR_error_string_n(unsigned long code, char *buffer, size_t length) { ERR_error_string_n(code, buffer, length); }
MS const char *ms_X509_verify_cert_error_string(long code) { return X509_verify_cert_error_string(code); }
