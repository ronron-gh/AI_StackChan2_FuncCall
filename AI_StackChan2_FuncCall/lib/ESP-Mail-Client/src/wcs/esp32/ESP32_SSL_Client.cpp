/*
 * ESP32 SSL Client v2.1.0
 *
 * Created July 27, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Provide SSL/TLS functions to ESP32 with Arduino IDE
 *
 * Adapted from the ssl_client1 example of mbedtls.
 *
 * Original Copyright (C) 2006-2015, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) 2017 Evandro Luis Copercini, Apache 2.0 License.
 */

#ifndef ESP32_SSL_Client_CPP
#define ESP32_SSL_Client_CPP

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if defined(ESP32) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#include <mbedtls/sha256.h>
#include <mbedtls/oid.h>
#include "ESP32_SSL_Client.h"

#if !defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED) && !defined(MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED)
#error "Please configure IDF framework to include mbedTLS -> Enable pre-shared-key ciphersuites and activate at least one cipher"
#endif

const char *esp32_ssl_str = "esp32-tls";

static int _esp32_ssl_handle_error(int err, const char *file, int line)
{
    if (err == -30848)
    {
        return err;
    }
#if !defined(SILENT_MODE)
#ifdef MBEDTLS_ERROR_C
    char error_buf[100];
    mbedtls_strerror(err, error_buf, 100);
    log_e("[%s():%d]: (%d) %s", file, line, err, error_buf);
#else
    log_e("[%s():%d]: code %d", file, line, err);
#endif
#endif
    return err;
}

#define esp32_ssl_handle_error(e) _esp32_ssl_handle_error(e, __FUNCTION__, __LINE__)

void ESP32_SSL_Client::ssl_init(ssl_ctx *ssl)
{
    // reset embedded pointers to zero
    memset(ssl, 0, sizeof(ssl_ctx));
    mbedtls_ssl_init(&ssl->ssl_ctx);
    mbedtls_ssl_config_init(&ssl->ssl_conf);
    mbedtls_ctr_drbg_init(&ssl->drbg_ctx);
}

#if defined(ESP_MAIL_ESP32_USE_WIFICLIENT_TEST)

namespace esp_mail_esp32_basic_client_io
{

    /**
     * \brief          Callback type: send data on the network.
     *
     * \note           That callback may be either blocking or non-blocking.
     *
     * \param ctx      Context for the send callback (typically a file descriptor)
     * \param buf      Buffer holding the data to send
     * \param len      Length of the data to send
     *
     * \return         The callback must return the number of bytes sent if any,
     *                 or a non-zero error code.
     *                 If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_WRITE
     *                 must be returned when the operation would block.
     *
     * \note           The callback is allowed to send fewer bytes than requested.
     *                 It must always return the number of bytes actually sent.
     */
    static int net_send(void *ctx, const unsigned char *buf, size_t len)
    {
        Client *client = (Client *)ctx;

        if (!client)
            return -1;

        int res = client->write(buf, len);

        return res;
    }

    /**
     * \brief          Callback type: receive data from the network, with timeout
     *
     * \note           That callback must block until data is received, or the
     *                 timeout delay expires, or the operation is interrupted by a
     *                 signal.
     *
     * \param ctx      Context for the receive callback (typically a file descriptor)
     * \param buf      Buffer to write the received data to
     * \param len      Length of the receive buffer
     * \param timeout  Maximum number of milliseconds to wait for data
     *                 0 means no timeout (potentially waiting forever)
     *
     * \return         The callback must return the number of bytes received,
     *                 or a non-zero error code:
     *                 \c MBEDTLS_ERR_SSL_TIMEOUT if the operation timed out,
     *                 \c MBEDTLS_ERR_SSL_WANT_READ if interrupted by a signal.
     *
     * \note           The callback may receive fewer bytes than the length of the
     *                 buffer. It must always return the number of bytes actually
     *                 received and written to the buffer.
     */
    static int net_recv_timeout(void *ctx, unsigned char *buf, size_t len, uint32_t timeout)
    {
        Client *basic_client = (Client *)ctx;

        if (!basic_client)
            return -1;

        int read = 0;

        if (len > 2048)
        {
            if (timeout == 0)
                timeout = 5000;
            unsigned long tm = millis();
            while (millis() - tm < timeout && read < len)
            {
                // This is required for ESP32 when continuousely reading large data or multiples chunks from slow SPI
                // basic client device, otherwise the connection may die at some point
                delay(0);
                int r = basic_client->read();
                if (r > -1)
                    buf[read++] = r;
            }
        }
        else
            read = basic_client->read(buf, len);

        if (read)
            return read;

        if (!basic_client->connected() && basic_client->available() == 0)
            return MBEDTLS_ERR_NET_CONN_RESET;

        return MBEDTLS_ERR_SSL_WANT_READ;
    }
};

#endif

int ESP32_SSL_Client::connect_ssl(ssl_ctx *ssl, const char *host, const char *rootCABuff, const char *cli_cert, const char *cli_key, const char *pskIdent, const char *psKey, bool insecure)
{

    if (!ssl->client)
        return -1;

#if !defined(SILENT_MODE)

    if (ssl->_debugCallback)
    {
        esp_mail_debug_print("", true);
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_27 /* "SSL/TLS negotiation" */, esp_mail_debug_tag_type_client);
    }
#endif

    if (rootCABuff == NULL && pskIdent == NULL && psKey == NULL && !insecure)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_8 /* "root certificate, PSK identity or keys are required for secured connection" */, esp_mail_debug_tag_type_error);
#endif
        return -1;
    }

    char buf[512];
    int ret, flags;
#if !defined(SILENT_MODE)
    if (ssl->_debugCallback)
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_9, esp_mail_debug_tag_type_client);
    log_v("Seeding the random number generator");
#endif

    mbedtls_entropy_init(&ssl->entropy_ctx);

    ret = mbedtls_ctr_drbg_seed(&ssl->drbg_ctx, mbedtls_entropy_func, &ssl->entropy_ctx, (const unsigned char *)esp32_ssl_str, strlen(esp32_ssl_str));
    if (ret < 0)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
        return esp32_ssl_handle_error(ret);
    }

#if !defined(SILENT_MODE)
    if (ssl->_debugCallback)
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_10, esp_mail_debug_tag_type_client);
    log_v("Setting up the SSL/TLS structure...");
#endif

    if ((ret = mbedtls_ssl_config_defaults(&ssl->ssl_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
        return esp32_ssl_handle_error(ret);
    }

    // MBEDTLS_SSL_VERIFY_REQUIRED if a CA certificate is defined and
    // MBEDTLS_SSL_VERIFY_NONE if not.

    if (insecure)
    {
        mbedtls_ssl_conf_authmode(&ssl->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_1 /* "Skipping SSL Verification. INSECURE!" */, esp_mail_debug_tag_type_warning);
        log_d("WARNING: Skipping SSL Verification. INSECURE!");
#endif
    }
    else if (rootCABuff != NULL)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_11 /* "Loading CA cert" */, esp_mail_debug_tag_type_client);
        log_v("Loading CA cert");
#endif

        mbedtls_x509_crt_init(&ssl->ca_cert);
        mbedtls_ssl_conf_authmode(&ssl->ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        ret = mbedtls_x509_crt_parse(&ssl->ca_cert, (const unsigned char *)rootCABuff, strlen(rootCABuff) + 1);
        mbedtls_ssl_conf_ca_chain(&ssl->ssl_conf, &ssl->ca_cert, NULL);
        // mbedtls_ssl_conf_verify(&ssl->ssl_ctx, my_verify, NULL );
        if (ret < 0)
        {
#if !defined(SILENT_MODE)
            if (ssl->_debugCallback)
                ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
            // free the ca_cert in the case parse failed, otherwise, the old ca_cert still in the heap memory, that lead to "out of memory" crash.
            mbedtls_x509_crt_free(&ssl->ca_cert);
            return esp32_ssl_handle_error(ret);
        }
    }
    else if (pskIdent != NULL && psKey != NULL)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_12 /* "Setting up PSK" */, esp_mail_debug_tag_type_client);
        log_v("Setting up PSK");
#endif
        // convert PSK from hex to binary
        if ((strlen(psKey) & 1) != 0 || strlen(psKey) > 2 * MBEDTLS_PSK_MAX_LEN)
        {
#if !defined(SILENT_MODE)
            if (ssl->_debugCallback)
                ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_13 /* "pre-shared key not valid hex or too long" */, esp_mail_debug_tag_type_error);
            log_e("pre-shared key not valid hex or too long");
#endif
            return -1;
        }

        unsigned char psk[MBEDTLS_PSK_MAX_LEN];
        size_t psk_len = strlen(psKey) / 2;
        for (int j = 0; j < strlen(psKey); j += 2)
        {
            char c = psKey[j];
            if (c >= '0' && c <= '9')
                c -= '0';
            else if (c >= 'A' && c <= 'F')
                c -= 'A' - 10;
            else if (c >= 'a' && c <= 'f')
                c -= 'a' - 10;
            else
                return -1;
            psk[j / 2] = c << 4;
            c = psKey[j + 1];
            if (c >= '0' && c <= '9')
                c -= '0';
            else if (c >= 'A' && c <= 'F')
                c -= 'A' - 10;
            else if (c >= 'a' && c <= 'f')
                c -= 'a' - 10;
            else
                return -1;
            psk[j / 2] |= c;
        }
// set mbedtls config
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_14 /* "set mbedtls config" */, esp_mail_debug_tag_type_client);
#endif
        ret = mbedtls_ssl_conf_psk(&ssl->ssl_conf, psk, psk_len,
                                   (const unsigned char *)pskIdent, strlen(pskIdent));
        if (ret != 0)
        {
#if !defined(SILENT_MODE)
            if (ssl->_debugCallback)
                ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
            log_e("mbedtls_ssl_conf_psk returned %d", ret);
#endif
            return esp32_ssl_handle_error(ret);
        }
    }
    else
    {
        return -1;
    }

    if (!insecure && cli_cert != NULL && cli_key != NULL)
    {
        mbedtls_x509_crt_init(&ssl->client_cert);
        mbedtls_pk_init(&ssl->client_key);

#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_15 /* "Loading CRT cert" */, esp_mail_debug_tag_type_client);
        log_v("Loading CRT cert");
#endif
        ret = mbedtls_x509_crt_parse(&ssl->client_cert, (const unsigned char *)cli_cert, strlen(cli_cert) + 1);
        if (ret < 0)
        {
#if !defined(SILENT_MODE)
            if (ssl->_debugCallback)
                ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
            // free the client_cert in the case parse failed, otherwise, the old client_cert still in the heap memory, that lead to "out of memory" crash.
            mbedtls_x509_crt_free(&ssl->client_cert);
            return esp32_ssl_handle_error(ret);
        }

#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_16 /* "Loading private key" */, esp_mail_debug_tag_type_client);
        log_v("Loading private key");
#endif
        ret = mbedtls_pk_parse_key(&ssl->client_key, (const unsigned char *)cli_key, strlen(cli_key) + 1, NULL, 0);

        if (ret != 0)
        {
#if !defined(SILENT_MODE)
            if (ssl->_debugCallback)
                ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
            mbedtls_x509_crt_free(&ssl->client_cert); // cert+key are free'd in pair
            return esp32_ssl_handle_error(ret);
        }

        mbedtls_ssl_conf_own_cert(&ssl->ssl_conf, &ssl->client_cert, &ssl->client_key);
    }
#if !defined(SILENT_MODE)
    if (ssl->_debugCallback)
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_17 /* "Setting hostname for TLS session..." */, esp_mail_debug_tag_type_client);
    log_v("Setting hostname for TLS session...");
#endif

    // Hostname set here should match CN in server certificate
    if ((ret = mbedtls_ssl_set_hostname(&ssl->ssl_ctx, host)) != 0)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
        return esp32_ssl_handle_error(ret);
    }

    mbedtls_ssl_conf_rng(&ssl->ssl_conf, mbedtls_ctr_drbg_random, &ssl->drbg_ctx);

    if ((ret = mbedtls_ssl_setup(&ssl->ssl_ctx, &ssl->ssl_conf)) != 0)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
        return esp32_ssl_handle_error(ret);
    }

#if defined(ESP_MAIL_ESP32_USE_WIFICLIENT_SOCKET_TEST)
    // Slowest
    ssl->socket = ssl->wc->fd();
    mbedtls_ssl_set_bio(&ssl->ssl_ctx, &ssl->socket, mbedtls_net_send, mbedtls_net_recv, NULL);
#elif defined(ESP_MAIL_ESP32_USE_WIFICLIENT_TEST)
    // Slow
    mbedtls_ssl_set_bio(&ssl->ssl_ctx, ssl->client, esp_mail_esp32_basic_client_io::net_send, NULL, esp_mail_esp32_basic_client_io::net_recv_timeout);
#else
    // Fastest
    mbedtls_ssl_set_bio(&ssl->ssl_ctx, &ssl->socket, mbedtls_net_send, mbedtls_net_recv, NULL);
#endif

#if !defined(SILENT_MODE)
    if (ssl->_debugCallback)
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_18 /* "Performing the SSL/TLS handshake..."*/, esp_mail_debug_tag_type_client);
    log_v("Performing the SSL/TLS handshake...");
#endif
    unsigned long handshake_start_time = millis();
    while ((ret = mbedtls_ssl_handshake(&ssl->ssl_ctx)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
#if !defined(SILENT_MODE)
            if (ssl->_debugCallback)
                ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
            return esp32_ssl_handle_error(ret);
        }
        if ((millis() - handshake_start_time) > ssl->handshake_timeout)
            return -1;

        vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    if (cli_cert != NULL && cli_key != NULL)
    {
#if !defined(SILENT_MODE)
        log_d("Protocol is %s Ciphersuite is %s", mbedtls_ssl_get_version(&ssl->ssl_ctx), mbedtls_ssl_get_ciphersuite(&ssl->ssl_ctx));

        if ((ret = mbedtls_ssl_get_record_expansion(&ssl->ssl_ctx)) >= 0)
        {
            log_d("Record expansion is %d", ret);
        }
        else
        {
            log_w("Record expansion is unknown (compression)");
        }
#endif
    }

#if !defined(SILENT_MODE)
    if (ssl->_debugCallback)
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_19, esp_mail_debug_tag_type_client);
    log_v("Verifying peer X.509 certificate...");
#endif

    if ((flags = mbedtls_ssl_get_verify_result(&ssl->ssl_ctx)) != 0)
    {
#if !defined(SILENT_MODE)
        memset(buf, 0, sizeof(buf));
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
        log_e("Failed to verify peer certificate! verification info: %s", buf);
#endif
        stop_tcp_connection(ssl, rootCABuff, cli_cert, cli_key); // It's not safe continue.
        return esp32_ssl_handle_error(ret);
    }
    else
    {
#if !defined(SILENT_MODE)
        log_v("Certificate verified.");
#endif
    }

    if (rootCABuff != NULL)
    {
        mbedtls_x509_crt_free(&ssl->ca_cert);
    }

    if (cli_cert != NULL)
    {
        mbedtls_x509_crt_free(&ssl->client_cert);
    }

    if (cli_key != NULL)
    {
        mbedtls_pk_free(&ssl->client_key);
    }
#if !defined(SILENT_MODE)
    log_v("Free internal heap after TLS %u", ESP.getFreeHeap());
#endif
    return 1;
}

void ESP32_SSL_Client::stop_tcp_connection(ssl_ctx *ssl, const char *rootCABuff, const char *cli_cert, const char *cli_key)
{

#if !defined(SILENT_MODE)
    if (ssl->_debugCallback)
        ssl_client_debug_pgm_send_cb(ssl, esp_ssl_client_str_22, esp_mail_debug_tag_type_client);

    log_v("Cleaning SSL connection.");
#endif

    mbedtls_ssl_free(&ssl->ssl_ctx);
    mbedtls_ssl_config_free(&ssl->ssl_conf);
    mbedtls_ctr_drbg_free(&ssl->drbg_ctx);
    mbedtls_entropy_free(&ssl->entropy_ctx);
}

int ESP32_SSL_Client::data_to_read(ssl_ctx *ssl)
{
    int ret, res;
    ret = mbedtls_ssl_read(&ssl->ssl_ctx, NULL, 0);
    // log_e("RET: %i",ret);   //for low level debug
    res = mbedtls_ssl_get_bytes_avail(&ssl->ssl_ctx);
    // log_e("RES: %i",res);    //for low level debug
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret < 0)
    {
#if !defined(SILENT_MODE)
        if (ssl->_debugCallback)
            ssl_client_send_mbedtls_error_cb(ssl, ret, esp_mail_debug_tag_type_error);
#endif
        return esp32_ssl_handle_error(ret);
    }

    return res;
}

int ESP32_SSL_Client::send_ssl_data(ssl_ctx *ssl, const uint8_t *data, size_t len)
{
#if !defined(SILENT_MODE)
    log_v("Writing request with %d bytes...", len); // for low level debug
#endif
    int ret = -1;

    while ((ret = mbedtls_ssl_write(&ssl->ssl_ctx, data, len)) <= 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret < 0)
        {
#if !defined(SILENT_MODE)
            log_v("Handling error %d", ret); // for low level debug
#endif
            return esp32_ssl_handle_error(ret);
        }
        // wait for space to become available
        vTaskDelay(2);
    }

    return ret;
}

int ESP32_SSL_Client::get_ssl_receive(ssl_ctx *ssl, uint8_t *data, int length)
{

    // log_d( "Reading HTTP response...");   //for low level debug
    int ret = -1;

    ret = mbedtls_ssl_read(&ssl->ssl_ctx, data, length);

    // log_v( "%d bytes read", ret);   //for low level debug
    return ret;
}

bool ESP32_SSL_Client::parseHexNibble(char pb, uint8_t *res)
{
    if (pb >= '0' && pb <= '9')
    {
        *res = (uint8_t)(pb - '0');
        return true;
    }
    else if (pb >= 'a' && pb <= 'f')
    {
        *res = (uint8_t)(pb - 'a' + 10);
        return true;
    }
    else if (pb >= 'A' && pb <= 'F')
    {
        *res = (uint8_t)(pb - 'A' + 10);
        return true;
    }
    return false;
}

// Compare a name from certificate and domain name, return true if they match
bool ESP32_SSL_Client::matchName(const std::string &name, const std::string &domainName)
{
    size_t wildcardPos = name.find('*');
    if (wildcardPos == std::string::npos)
    {
        // Not a wildcard, expect an exact match
        return name == domainName;
    }

    size_t firstDotPos = name.find('.');
    if (wildcardPos > firstDotPos)
    {
        // Wildcard is not part of leftmost component of domain name
        // Do not attempt to match (rfc6125 6.4.3.1)
        return false;
    }
    if (wildcardPos != 0 || firstDotPos != 1)
    {
        // Matching of wildcards such as baz*.example.com and b*z.example.com
        // is optional. Maybe implement this in the future?
        return false;
    }
    size_t domainNameFirstDotPos = domainName.find('.');
    if (domainNameFirstDotPos == std::string::npos)
    {
        return false;
    }
    return domainName.substr(domainNameFirstDotPos) == name.substr(firstDotPos);
}

// Verifies certificate provided by the peer to match specified SHA256 fingerprint
bool ESP32_SSL_Client::verify_ssl_fingerprint(ssl_ctx *ssl, const char *fp, const char *domain_name)
{
    // Convert hex string to byte array
    uint8_t fingerprint_local[32];
    int len = strlen(fp);
    int pos = 0;
    for (size_t i = 0; i < sizeof(fingerprint_local); ++i)
    {
        while (pos < len && ((fp[pos] == ' ') || (fp[pos] == ':')))
        {
            ++pos;
        }
        if (pos > len - 2)
        {
#if !defined(SILENT_MODE)
            log_d("pos:%d len:%d fingerprint too short", pos, len);
#endif
            return false;
        }
        uint8_t high, low;
        if (!parseHexNibble(fp[pos], &high) || !parseHexNibble(fp[pos + 1], &low))
        {
#if !defined(SILENT_MODE)
            log_d("pos:%d len:%d invalid hex sequence: %c%c", pos, len, fp[pos], fp[pos + 1]);
#endif
            return false;
        }
        pos += 2;
        fingerprint_local[i] = low | (high << 4);
    }

    // Get certificate provided by the peer
    const mbedtls_x509_crt *crt = mbedtls_ssl_get_peer_cert(&ssl->ssl_ctx);

    if (!crt)
    {
#if !defined(SILENT_MODE)
        log_d("could not fetch peer certificate");
#endif
        return false;
    }

    // Calculate certificate's SHA256 fingerprint
    uint8_t fingerprint_remote[32];
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, false);
    mbedtls_sha256_update(&sha256_ctx, crt->raw.p, crt->raw.len);
    mbedtls_sha256_finish(&sha256_ctx, fingerprint_remote);

    // Check if fingerprints match
    if (memcmp(fingerprint_local, fingerprint_remote, 32))
    {
#if !defined(SILENT_MODE)
        log_d("fingerprint doesn't match");
#endif
        return false;
    }

    // Additionally check if certificate has domain name if provided
    if (domain_name)
        return verify_ssl_dn(ssl, domain_name);
    else
        return true;
}

// Checks if peer certificate has specified domain in CN or SANs
bool ESP32_SSL_Client::verify_ssl_dn(ssl_ctx *ssl, const char *domain_name)
{
#if !defined(SILENT_MODE)
    log_d("domain name: '%s'", (domain_name) ? domain_name : "(null)");
#endif
    std::string domain_name_str(domain_name);
    std::transform(domain_name_str.begin(), domain_name_str.end(), domain_name_str.begin(), ::tolower);

    // Get certificate provided by the peer
    const mbedtls_x509_crt *crt = mbedtls_ssl_get_peer_cert(&ssl->ssl_ctx);

    // Check for domain name in SANs
    const mbedtls_x509_sequence *san = &crt->subject_alt_names;
    while (san != nullptr)
    {
        std::string san_str((const char *)san->buf.p, san->buf.len);
        std::transform(san_str.begin(), san_str.end(), san_str.begin(), ::tolower);

        if (matchName(san_str, domain_name_str))
            return true;

#if !defined(SILENT_MODE)
        log_d("SAN '%s': no match", san_str.c_str());
#endif
        // Fetch next SAN
        san = san->next;
    }

    // Check for domain name in CN
    const mbedtls_asn1_named_data *common_name = &crt->subject;
    while (common_name != nullptr)
    {
        // While iterating through DN objects, check for CN object
        if (!MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &common_name->oid))
        {
            std::string common_name_str((const char *)common_name->val.p, common_name->val.len);

            if (matchName(common_name_str, domain_name_str))
                return true;
#if !defined(SILENT_MODE)
            log_d("CN '%s': not match", common_name_str.c_str());
#endif
        }

        // Fetch next DN object
        common_name = common_name->next;
    }

    return false;
}

#endif // ESP32

#endif // ESP32_SSL_Client_CPP