/**
 * Copyright (c) [2019] maminjie <canpool@163.com>
 *
 * vlink is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *    http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#ifdef WITH_DTLS
#include "dtls.h"
/* timeout for udp client shakehand,the unit is second */
#ifndef DTLS_UDP_CLIENT_SHAKEHAND_TIMEOUT
#define DTLS_UDP_CLIENT_SHAKEHAND_TIMEOUT 60
#endif

#endif

#include <coap_al.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "coap.h"
#include "utlist.h"

#include "vos.h"

extern int g_bind_finsh;

static cmd_dealer_fn cmd_func = NULL;

int flags = 0;

static unsigned char _token_data[8] = "cafe";
coap_binary_t the_token = {8, _token_data};

#define FLAGS_BLOCK 0x01

static coap_optlist_t *s_optlist = NULL;

/* reading is done when this flag is set */
static int ready = 0;

static coap_string_t payload = {0, NULL}; /* optional payload to send */

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

typedef unsigned char method_t;
method_t method = 1; /* the method we are using in our requests */

coap_block_t block = {.num = 0, .m = 0, .szx = 6};
uint16_t last_block1_tid = 0;

unsigned int wait_seconds = 90; /* default timeout in seconds */
unsigned int wait_ms = 0;
int wait_ms_reset = 0;
int obs_started = 0;
unsigned int obs_seconds = 30; /* default observe time */
unsigned int obs_ms = 0;       /* timeout for current subscription */
int obs_ms_reset = 0;

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

static int libcoap_generate_token(unsigned char *token)
{
    unsigned int ret;
    srand((unsigned)vos_sys_time());
    ret = rand() % RAND_MAX;
    token[0] = (unsigned char)ret;
    token[1] = (unsigned char)(ret >> 8);
    token[2] = (unsigned char)(ret >> 16);
    token[3] = (unsigned char)(ret >> 24);
    return 8;
}

static void handle_observe_request(coap_context_t *ctx, coap_resource_t *resource, coap_session_t *session UNUSED_PARAM,
                                   coap_pdu_t *request, coap_binary_t *token UNUSED_PARAM,
                                   coap_string_t *query UNUSED_PARAM, coap_pdu_t *response)
{
    unsigned char buf[3];

    // need record token (take care)
    memcpy(the_token.s, response->token, response->token_length);
    the_token.length = response->token_length;

    response->code = COAP_RESPONSE_CODE(204);

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_safe(buf, sizeof(buf), 65535), buf);
}

static void handle_cmd_request(coap_context_t *ctx UNUSED_PARAM, coap_resource_t *resource, coap_session_t *session,
                               coap_pdu_t *request, coap_binary_t *token, coap_string_t *query UNUSED_PARAM,
                               coap_pdu_t *response)
{
    if (cmd_func != NULL && request->data != NULL) {
        cmd_func((char *)request->data, request->used_size);
    }
    response->code = COAP_RESPONSE_CODE(204);
}

static void init_resources(coap_context_t *ctx)
{
    coap_resource_t *r;

    r = coap_resource_init(coap_make_str_const("t/d"), 0);
    coap_register_handler(r, COAP_REQUEST_GET, handle_observe_request);
    coap_register_handler(r, COAP_REQUEST_POST, handle_cmd_request);
    coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);

    coap_add_resource(ctx, r);
}

static int append_to_output(const uint8_t *data, size_t len)
{
    return 0;
}

static coap_pdu_t *coap_new_request(coap_context_t *ctx, coap_session_t *session, method_t m, coap_optlist_t **options,
                                    unsigned char *data, size_t length)
{
    coap_pdu_t *pdu;
    (void)ctx;

    if (!(pdu = coap_new_pdu(session)))
        return NULL;

    pdu->type = msgtype;
    pdu->tid = coap_new_message_id(session);
    pdu->code = m;

    if (m != COAP_RESPONSE_CODE(205)) {
        libcoap_generate_token((unsigned char *)the_token.s);
    }

    if (!coap_add_token(pdu, the_token.length, the_token.s)) {
        coap_log(LOG_DEBUG, "cannot add token to request\n");
    }

    if (options)
        coap_add_optlist_pdu(pdu, options);

    if (length) {
        if ((flags & FLAGS_BLOCK) == 0)
            coap_add_data(pdu, length, data);
        else
            coap_add_block(pdu, length, data, block.num, block.szx);
    }

    return pdu;
}

static coap_tid_t clear_obs(coap_context_t *ctx, coap_session_t *session)
{
    coap_pdu_t *pdu;
    coap_optlist_t *option;
    coap_tid_t tid = COAP_INVALID_TID;
    unsigned char buf[2];
    (void)ctx;

    /* create bare PDU w/o any option  */
    pdu = coap_pdu_init(msgtype, COAP_REQUEST_GET, coap_new_message_id(session), coap_session_max_pdu_size(session));

    if (!pdu) {
        return tid;
    }

    if (!coap_add_token(pdu, the_token.length, the_token.s)) {
        coap_log(LOG_CRIT, "cannot add token\n");
        goto error;
    }

    for (option = s_optlist; option; option = option->next) {
        if (option->number == COAP_OPTION_URI_HOST) {
            if (!coap_add_option(pdu, option->number, option->length, option->data)) {
                goto error;
            }
            break;
        }
    }

    if (!coap_add_option(pdu, COAP_OPTION_OBSERVE, coap_encode_var_safe(buf, sizeof(buf), COAP_OBSERVE_CANCEL), buf)) {
        coap_log(LOG_CRIT, "cannot add option Observe: %u\n", COAP_OBSERVE_CANCEL);
        goto error;
    }

    for (option = s_optlist; option; option = option->next) {
        switch (option->number) {
        case COAP_OPTION_URI_PORT:
        case COAP_OPTION_URI_PATH:
        case COAP_OPTION_URI_QUERY:
            if (!coap_add_option(pdu, option->number, option->length, option->data)) {
                goto error;
            }
            break;
        default:;
        }
    }

    if (flags & FLAGS_BLOCK) {
        block.num = 0;
        block.m = 0;
        coap_add_option(pdu, COAP_OPTION_BLOCK2,
                        coap_encode_var_safe(buf, sizeof(buf), (block.num << 4 | block.m << 3 | block.szx)), buf);
    }

    if (coap_get_log_level() < LOG_DEBUG)
        coap_show_pdu(LOG_INFO, pdu);

    tid = coap_send(session, pdu);

    if (tid == COAP_INVALID_TID)
        coap_log(LOG_DEBUG, "clear_obs: error sending new request\n");

    return tid;
error:

    coap_delete_pdu(pdu);
    return tid;
}

#define HANDLE_BLOCK1(Pdu)                                                                                             \
    ((method == COAP_REQUEST_PUT || method == COAP_REQUEST_POST) && ((flags & FLAGS_BLOCK) == 0) &&                    \
     ((Pdu)->hdr->code == COAP_RESPONSE_CODE(201) || (Pdu)->hdr->code == COAP_RESPONSE_CODE(204)))

static inline int check_token(coap_pdu_t *received)
{
    return received->token_length == the_token.length && memcmp(received->token, the_token.s, the_token.length) == 0;
}

unsigned int coap_write(coap_context_t *ctx, coap_socket_t *sockets[], unsigned int max_sockets,
                        unsigned int *num_sockets, coap_tick_t now)
{
    coap_queue_t *nextpdu;
    coap_endpoint_t *ep;
    coap_session_t *s;
    coap_tick_t session_timeout;
    coap_tick_t timeout = 0;
    coap_session_t *tmp;

    *num_sockets = 0;

    /* Check to see if we need to send off any Observe requests */
    coap_check_notify(ctx);

    if (ctx->session_timeout > 0)
        session_timeout = ctx->session_timeout * COAP_TICKS_PER_SECOND;
    else
        session_timeout = COAP_DEFAULT_SESSION_TIMEOUT * COAP_TICKS_PER_SECOND;

    LL_FOREACH(ctx->endpoint, ep)
    {
        if (ep->sock.flags & (COAP_SOCKET_WANT_READ | COAP_SOCKET_WANT_WRITE | COAP_SOCKET_WANT_ACCEPT)) {
            if (*num_sockets < max_sockets)
                sockets[(*num_sockets)++] = &ep->sock;
        }
        LL_FOREACH_SAFE(ep->sessions, s, tmp)
        {
            if (s->type == COAP_SESSION_TYPE_SERVER && s->ref == 0 && s->delayqueue == NULL &&
                (s->last_rx_tx + session_timeout <= now || s->state == COAP_SESSION_STATE_NONE)) {
                coap_session_free(s);
            } else {
                if (s->type == COAP_SESSION_TYPE_SERVER && s->ref == 0 && s->delayqueue == NULL) {
                    coap_tick_t s_timeout = (s->last_rx_tx + session_timeout) - now;
                    if (timeout == 0 || s_timeout < timeout)
                        timeout = s_timeout;
                }
                if (s->sock.flags & (COAP_SOCKET_WANT_READ | COAP_SOCKET_WANT_WRITE)) {
                    if (*num_sockets < max_sockets)
                        sockets[(*num_sockets)++] = &s->sock;
                }
            }
        }
    }
    LL_FOREACH_SAFE(ctx->sessions, s, tmp)
    {
        if (s->type == COAP_SESSION_TYPE_CLIENT && COAP_PROTO_RELIABLE(s->proto) &&
            s->state == COAP_SESSION_STATE_ESTABLISHED && ctx->ping_timeout > 0) {
            coap_tick_t s_timeout;
            if (s->last_rx_tx + ctx->ping_timeout * COAP_TICKS_PER_SECOND <= now) {
                if ((s->last_ping > 0 && s->last_pong < s->last_ping) ||
                    coap_session_send_ping(s) == COAP_INVALID_TID) {
                    /* Make sure the session object is not deleted in the callback */
                    coap_session_reference(s);
                    coap_session_disconnected(s, COAP_NACK_NOT_DELIVERABLE);
                    coap_session_release(s);
                    continue;
                }
                s->last_rx_tx = now;
                s->last_ping = now;
            }
            s_timeout = (s->last_rx_tx + ctx->ping_timeout * COAP_TICKS_PER_SECOND) - now;
            if (timeout == 0 || s_timeout < timeout)
                timeout = s_timeout;
        }

        if (s->type == COAP_SESSION_TYPE_CLIENT && COAP_PROTO_RELIABLE(s->proto) &&
            s->state == COAP_SESSION_STATE_CSM && ctx->csm_timeout > 0) {
            coap_tick_t s_timeout;
            if (s->csm_tx == 0) {
                s->csm_tx = now;
            } else if (s->csm_tx + ctx->csm_timeout * COAP_TICKS_PER_SECOND <= now) {
                /* Make sure the session object is not deleted in the callback */
                coap_session_reference(s);
                coap_session_disconnected(s, COAP_NACK_NOT_DELIVERABLE);
                coap_session_release(s);
                continue;
            }
            s_timeout = (s->csm_tx + ctx->csm_timeout * COAP_TICKS_PER_SECOND) - now;
            if (timeout == 0 || s_timeout < timeout)
                timeout = s_timeout;
        }

        if (s->sock.flags & (COAP_SOCKET_WANT_READ | COAP_SOCKET_WANT_WRITE | COAP_SOCKET_WANT_CONNECT)) {
            if (*num_sockets < max_sockets)
                sockets[(*num_sockets)++] = &s->sock;
        }
    }

    nextpdu = coap_peek_next(ctx);

    while (nextpdu && now >= ctx->sendqueue_basetime && nextpdu->t <= now - ctx->sendqueue_basetime) {
        coap_retransmit(ctx, coap_pop_next(ctx));
        nextpdu = coap_peek_next(ctx);
    }

    if (nextpdu && (timeout == 0 || nextpdu->t - (now - ctx->sendqueue_basetime) < timeout))
        timeout = nextpdu->t - (now - ctx->sendqueue_basetime);

    if (ctx->dtls_context) {
        if (coap_dtls_is_context_timeout()) {
            coap_tick_t tls_timeout = coap_dtls_get_context_timeout(ctx->dtls_context);
            if (tls_timeout > 0) {
                if (tls_timeout < now + COAP_TICKS_PER_SECOND / 10)
                    tls_timeout = now + COAP_TICKS_PER_SECOND / 10;
                coap_log(LOG_DEBUG, "** DTLS global timeout set to %dms\n",
                         (int)((tls_timeout - now) * 1000 / COAP_TICKS_PER_SECOND));
                if (timeout == 0 || tls_timeout - now < timeout)
                    timeout = tls_timeout - now;
            }
        } else {
            LL_FOREACH(ctx->endpoint, ep)
            {
                if (ep->proto == COAP_PROTO_DTLS) {
                    LL_FOREACH(ep->sessions, s)
                    {
                        if (s->proto == COAP_PROTO_DTLS && s->tls) {
                            coap_tick_t tls_timeout = coap_dtls_get_timeout(s);
                            while (tls_timeout > 0 && tls_timeout <= now) {
                                coap_log(LOG_DEBUG, "** %s: DTLS retransmit timeout\n", coap_session_str(s));
                                coap_dtls_handle_timeout(s);
                                if (s->tls)
                                    tls_timeout = coap_dtls_get_timeout(s);
                                else {
                                    tls_timeout = 0;
                                    timeout = 1;
                                }
                            }
                            if (tls_timeout > 0 && (timeout == 0 || tls_timeout - now < timeout))
                                timeout = tls_timeout - now;
                        }
                    }
                }
            }
            LL_FOREACH(ctx->sessions, s)
            {
                if (s->proto == COAP_PROTO_DTLS && s->tls) {
                    coap_tick_t tls_timeout = coap_dtls_get_timeout(s);
                    while (tls_timeout > 0 && tls_timeout <= now) {
                        coap_log(LOG_DEBUG, "** %s: DTLS retransmit timeout\n", coap_session_str(s));
                        coap_dtls_handle_timeout(s);
                        if (s->tls)
                            tls_timeout = coap_dtls_get_timeout(s);
                        else {
                            tls_timeout = 0;
                            timeout = 1;
                        }
                    }
                    if (tls_timeout > 0 && (timeout == 0 || tls_timeout - now < timeout))
                        timeout = tls_timeout - now;
                }
            }
        }
    }

    return (unsigned int)((timeout * 1000 + COAP_TICKS_PER_SECOND - 1) / COAP_TICKS_PER_SECOND);
}

static void message_handler(struct coap_context_t *ctx, coap_session_t *session, coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id)
{

    coap_pdu_t *pdu = NULL;
    coap_opt_t *block_opt;
    coap_opt_iterator_t opt_iter;
    unsigned char buf[4];
    coap_optlist_t *option;
    size_t len;
    unsigned char *databuf;
    coap_tid_t tid;

#ifndef NDEBUG
    coap_log(LOG_DEBUG, "** process incoming %d.%02d response:\n", (received->code >> 5), received->code & 0x1F);
    if (coap_get_log_level() < LOG_DEBUG)
        coap_show_pdu(LOG_INFO, received);
#endif

    /* check if this is a response to our original request */
    if (!check_token(received)) {
        /* drop if this was just some message, or send RST in case of notification */
        if (!sent && (received->type == COAP_MESSAGE_CON || received->type == COAP_MESSAGE_NON))
            coap_send_rst(session, received);
        return;
    }

    if (received->type == COAP_MESSAGE_RST) {
        coap_log(LOG_INFO, "got RST\n");
        return;
    }

    /* output the received data, if any */
    if (COAP_RESPONSE_CLASS(received->code) == 2) {

        /* set obs timer if we have successfully subscribed a resource */
        if (!obs_started && coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter)) {
            coap_log(LOG_DEBUG, "observation relationship established, set timeout to %d\n", obs_seconds);
            obs_started = 1;
            obs_ms = obs_seconds * 1000;
            obs_ms_reset = 1;
        }

        /* Got some data, check if block option is set. Behavior is undefined if
         * both, Block1 and Block2 are present. */
        block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
        if (block_opt) { /* handle Block2 */
            uint16_t blktype = opt_iter.type;

            /* TODO: check if we are looking at the correct block number */
            if (coap_get_data(received, &len, &databuf))
                append_to_output(databuf, len);

            if (coap_opt_block_num(block_opt) == 0) {
                /* See if observe is set in first response */
                g_bind_finsh = ready = coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter) == NULL;
            }
            if (COAP_OPT_BLOCK_MORE(block_opt)) {
                /* more bit is set */
                coap_log(LOG_DEBUG, "found the M bit, block size is %u, block nr. %u\n", COAP_OPT_BLOCK_SZX(block_opt),
                         coap_opt_block_num(block_opt));

                /* create pdu with request for next block */
                pdu =
                    coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
                if (pdu) {
                    /* add URI components from optlist */
                    for (option = s_optlist; option; option = option->next) {
                        switch (option->number) {
                        case COAP_OPTION_URI_HOST:
                        case COAP_OPTION_URI_PORT:
                        case COAP_OPTION_URI_PATH:
                        case COAP_OPTION_URI_QUERY:
                            coap_add_option(pdu, option->number, option->length, option->data);
                            break;
                        default:; /* skip other options */
                        }
                    }

                    /* finally add updated block option from response, clear M bit */
                    /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
                    coap_log(LOG_DEBUG, "query block %d\n", (coap_opt_block_num(block_opt) + 1));
                    coap_add_option(pdu, blktype,
                                    coap_encode_var_safe(buf, sizeof(buf),
                                                         ((coap_opt_block_num(block_opt) + 1) << 4) |
                                                             COAP_OPT_BLOCK_SZX(block_opt)),
                                    buf);

                    tid = coap_send(session, pdu);

                    if (tid == COAP_INVALID_TID) {
                        coap_log(LOG_DEBUG, "message_handler: error sending new request\n");
                    } else {
                        wait_ms = wait_seconds * 1000;
                        wait_ms_reset = 1;
                    }

                    return;
                }
            }
            return;
        } else { /* no Block2 option */
            block_opt = coap_check_option(received, COAP_OPTION_BLOCK1, &opt_iter);

            if (block_opt) { /* handle Block1 */
                unsigned int szx = COAP_OPT_BLOCK_SZX(block_opt);
                unsigned int num = coap_opt_block_num(block_opt);
                coap_log(LOG_DEBUG, "found Block1 option, block size is %u, block nr. %u\n", szx, num);
                if (szx != block.szx) {
                    unsigned int bytes_sent = ((block.num + 1) << (block.szx + 4));
                    if (bytes_sent % (1 << (szx + 4)) == 0) {
                        /* Recompute the block number of the previous packet given the new block size */
                        num = block.num = (bytes_sent >> (szx + 4)) - 1;
                        block.szx = szx;
                        coap_log(LOG_DEBUG, "new Block1 size is %u, block number %u completed\n",
                                 (1 << (block.szx + 4)), block.num);
                    } else {
                        coap_log(LOG_DEBUG,
                                 "ignoring request to increase Block1 size, "
                                 "next block is not aligned on requested block size boundary. "
                                 "(%u x %u mod %u = %u != 0)\n",
                                 block.num + 1, (1 << (block.szx + 4)), (1 << (szx + 4)),
                                 bytes_sent % (1 << (szx + 4)));
                    }
                }

                if (payload.length <= (block.num + 1) * (1 << (block.szx + 4))) {
                    coap_log(LOG_DEBUG, "upload ready\n");
                    g_bind_finsh = ready = 1;
                    return;
                }
                if (last_block1_tid == received->tid) {
                    /*
                     * Duplicate BLOCK1 ACK
                     *
                     * RFCs not clear here, but on a lossy connection, there could
                     * be multiple BLOCK1 ACKs, causing the client to retransmit the
                     * same block multiple times.
                     *
                     * Once a block has been ACKd, there is no need to retransmit it.
                     */
                    return;
                }
                last_block1_tid = received->tid;

                /* create pdu with request for next block */
                pdu =
                    coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
                if (pdu) {

                    /* add URI components from optlist */
                    for (option = s_optlist; option; option = option->next) {
                        switch (option->number) {
                        case COAP_OPTION_URI_HOST:
                        case COAP_OPTION_URI_PORT:
                        case COAP_OPTION_URI_PATH:
                        case COAP_OPTION_CONTENT_FORMAT:
                        case COAP_OPTION_URI_QUERY:
                            coap_add_option(pdu, option->number, option->length, option->data);
                            break;
                        default:; /* skip other options */
                        }
                    }

                    /* finally add updated block option from response, clear M bit */
                    /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
                    block.num = num + 1;
                    block.m = ((block.num + 1) * (1 << (block.szx + 4)) < payload.length);

                    coap_log(LOG_DEBUG, "send block %d\n", block.num);
                    coap_add_option(
                        pdu, COAP_OPTION_BLOCK1,
                        coap_encode_var_safe(buf, sizeof(buf), (block.num << 4) | (block.m << 3) | block.szx), buf);

                    coap_add_block(pdu, payload.length, payload.s, block.num, block.szx);
                    if (coap_get_log_level() < LOG_DEBUG)
                        coap_show_pdu(LOG_INFO, pdu);

                    tid = coap_send(session, pdu);

                    if (tid == COAP_INVALID_TID) {
                        coap_log(LOG_DEBUG, "message_handler: error sending new request\n");
                    } else {
                        wait_ms = wait_seconds * 1000;
                        wait_ms_reset = 1;
                    }

                    return;
                }
            } else {
                /* There is no block option set, just read the data and we are done. */
                if (coap_get_data(received, &len, &databuf))
                    append_to_output(databuf, len);
            }
        }
    } else { /* no 2.05 */

        /* check if an error was signaled and output payload if so */
        if (COAP_RESPONSE_CLASS(received->code) >= 4) {
            fprintf(stderr, "%d.%02d", (received->code >> 5), received->code & 0x1F);
            if (coap_get_data(received, &len, &databuf)) {
                fprintf(stderr, " ");
                while (len--)
                    fprintf(stderr, "%c", *databuf++);
            }
            fprintf(stderr, "\n");
        }
    }

    /* any pdu that has been created in this function must be sent by now */
    assert(pdu == NULL);

    /* our job is done, we can exit at any time */
    g_bind_finsh = ready = coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter) == NULL;
}

#ifdef WITH_DTLS
static dtls_context *get_dtls(coap_al_config_t *config)
{
    int ret;
    dtls_config_info config_info;
    dtls_handshake_info handshake_info;
    dtls_context *context = NULL;

    memset(&config_info, 0, sizeof(dtls_config_info));
    memset(&handshake_info, 0, sizeof(dtls_handshake_info));

    config_info.auth_type |= DTLS_AUTH_PSK;
    config_info.proto = MBEDTLS_NET_PROTO_UDP;
    config_info.endpoint = MBEDTLS_SSL_IS_CLIENT;
    config_info.pers = "coap_dtls_client";
    config_info.psk.psk_key = config->psk_key;
    config_info.psk.psk_key_len = config->psk_key_len;
    config_info.psk.psk_identity = config->psk_identity;
    config_info.psk.psk_identity_len = config->psk_identity_len;

    if ((context = dtls_create(&config_info)) == NULL) {
        return NULL;
    }
    if ((ret = dtls_connect(context, config->host, config->port)) != 0) {
        goto EXIT_FAIL;
    }
    handshake_info.timeout = CONFIG_DTLS_HANDSHAKE_TIMEOUT;
    if ((ret = dtls_handshake(context, &handshake_info)) != 0) {
        goto EXIT_FAIL;
    }

    return context;

EXIT_FAIL:
    dtls_destroy(context);
    return NULL;
}
#endif

static coap_session_t *get_session(coap_context_t *ctx, coap_al_config_t *config)
{
    coap_session_t *session = NULL;
    coap_address_t server;

    server.size = sizeof(server.addr);
    server.addr.sin.sin_addr.s_addr = inet_addr(config->host);
    server.addr.sin.sin_port = htons(atoi(config->port));
    server.addr.sin.sin_family = AF_INET;

    session = coap_new_client_session(ctx, NULL, &server, COAP_PROTO_UDP);

    return session;
}

static int __init(coaper_t *coaper, coap_al_config_t *config)
{
    coap_context_t *ctx = NULL;
    coap_session_t *session = NULL;

    coap_set_log_level(LOG_ERR);

#ifdef WITH_DTLS
    dtls_context *dtls = get_dtls(config);
    if (dtls == NULL) {
        coap_log(LOG_ERR, "get_dtls() failed!\r\n");
        return -1;
    }
#endif

    ctx = coap_new_context(NULL);
    if (ctx == NULL) {
        coap_log(LOG_ERR, "coap_new_context() failed!\r\n");
        goto EXIT_FAIL;
    }

    session = get_session(ctx, config);
    if (session == NULL) {
        coap_log(LOG_ERR, "get_session() failed!\r\n");
        goto EXIT_FAIL;
    }

    coap_register_option(ctx, COAP_OPTION_BLOCK2);
    coap_register_response_handler(ctx, message_handler);

    init_resources(ctx);

    wait_ms = wait_seconds * 1000;
    coap_log(LOG_DEBUG, "timeout is set to %u seconds\n", wait_seconds);

    cmd_func = config->dealer;

    coaper->ctx = ctx;
    coaper->session = session;
#ifdef WITH_DTLS
    ctx->dtls_context = dtls;
    coaper->ssl = dtls;
#endif

    return 0;

EXIT_FAIL:
    if (ctx != NULL) {
        coap_free_context(ctx);
    }
#ifdef WITH_DTLS
    if (dtls != NULL) {
        dtls_destroy(dtls);
    }
#endif
    if (session != NULL) {
        coap_session_free(session);
    }
    return -1;
}

static int __destroy(coaper_t *coaper)
{
    coap_context_t *ctx = (coap_context_t *)coaper->ctx;
#ifdef WITH_DTLS
    dtls_context *dtls = (dtls_context *)coaper->ssl;
#endif
    coap_optlist_t *optlist = (coap_optlist_t *)coaper->optlist;
    coap_pdu_t *pdu = (coap_pdu_t *)coaper->packet;

    if (ctx != NULL) {
        coap_free_context(ctx); // free session at the same time
    }
#ifdef WITH_DTLS
    if (dtls != NULL) {
        dtls_destroy(dtls);
    }
#endif
    if (optlist != NULL) {
        coap_delete_optlist(optlist);
    }
    if (pdu != NULL) {
        coap_delete_pdu(pdu);
    }
    memset(coaper, 0, sizeof(coaper_t));

    if (s_optlist != NULL) {
        coap_delete_optlist(s_optlist);
        s_optlist = NULL;
    }

    return 0;
}

static int __add_option(coaper_t *coaper, uint16_t number, size_t len, const uint8_t *data)
{
    int ret = coap_insert_optlist((coap_optlist_t **)&coaper->optlist, coap_new_optlist(number, len, data));

    return ret ? 0 : -1;
}

static int __request(coaper_t *coaper, uint8_t msg_type, uint8_t code, uint8_t *payload, size_t len)
{
    int ret = 0;
    coap_pdu_t *msg = NULL;

    msgtype = msg_type;
    msg = coap_new_request((coap_context_t *)coaper->ctx, (coap_session_t *)coaper->session, code,
                           (coap_optlist_t **)&coaper->optlist, payload, len);
    if (msg == NULL) {
        coap_delete_optlist((coap_optlist_t *)coaper->optlist);
        ret = -1;
    } else {
        if (msg_type == COAP_MESSAGE_CON) {
            if (s_optlist != NULL) {
                coap_delete_optlist(s_optlist);
            }
            s_optlist = (coap_optlist_t *)coaper->optlist;
        } else {
            coap_delete_optlist((coap_optlist_t *)coaper->optlist);
        }
    }
    coaper->optlist = NULL;
    coaper->packet = msg;

    return ret;
}

static int __send(coaper_t *coaper)
{
    int ret = -1;
    coap_pdu_t *msg = (coap_pdu_t *)coaper->packet;

    if (msg != NULL) {
        ret = coap_send((coap_session_t *)coaper->session, msg);
        if (ret == -1) {
            coap_log(LOG_ERR, "coap_send() failed\n");
        }
        coaper->packet = NULL;
    }

    return ret;
}

static int __timeout_check(coaper_t *coaper, int delay_ms)
{
    int ret = 0;
    if (delay_ms >= 0) {
        if (wait_ms > 0 && !wait_ms_reset) {
            if ((unsigned)delay_ms >= wait_ms) {
                coap_log(LOG_INFO, "timeout\n");
                ret = 1;
            } else {
                wait_ms -= delay_ms;
            }
        }
        if (obs_ms > 0 && !obs_ms_reset) {
            if ((unsigned)delay_ms >= obs_ms) {
                coap_log(LOG_DEBUG, "clear observation relationship\n");
                /* FIXME: handle error case COAP_TID_INVALID */
                clear_obs((coap_context_t *)coaper->ctx, (coap_session_t *)coaper->session);

                /* make sure that the obs timer does not fire again */
                obs_ms = 0;
                obs_seconds = 0;
                ret = 1;
            } else {
                obs_ms -= delay_ms;
            }
        }
        wait_ms_reset = 0;
        obs_ms_reset = 0;
    }
    return ret;
}

static int __recv(coaper_t *coaper)
{
    coap_tick_t before;
    coap_socket_t *sockets[64];
    unsigned int num_sockets = 0, i, timeout;
    unsigned int timeout_ms = (wait_ms == 0 ? obs_ms : obs_ms == 0 ? min(wait_ms, 1000) : min(wait_ms, obs_ms));
    coap_context_t *ctx = (coap_context_t *)coaper->ctx;

    coap_ticks(&before);

    timeout = coap_write(ctx, sockets, (unsigned int)(sizeof(sockets) / sizeof(sockets[0])), &num_sockets, before);
    if (timeout == 0 || timeout_ms < timeout)
        timeout = timeout_ms;

    for (i = 0; i < num_sockets; i++) {
        if ((sockets[i]->flags & COAP_SOCKET_WANT_READ))
            sockets[i]->flags |= COAP_SOCKET_CAN_READ;
        if ((sockets[i]->flags & COAP_SOCKET_WANT_ACCEPT))
            sockets[i]->flags |= COAP_SOCKET_CAN_ACCEPT;
        if ((sockets[i]->flags & COAP_SOCKET_WANT_WRITE))
            sockets[i]->flags |= COAP_SOCKET_CAN_WRITE;
        if ((sockets[i]->flags & COAP_SOCKET_WANT_CONNECT))
            sockets[i]->flags |= COAP_SOCKET_CAN_CONNECT;
    }

    coap_ticks((coap_tick_t *)&coaper->now);
    coap_read(ctx, (coap_tick_t)coaper->now);

    int delay_ms = (((coap_tick_t)coaper->now - before) * 1000) / COAP_TICKS_PER_SECOND;
    if (__timeout_check(coaper, delay_ms)) {
        coap_delete_optlist(s_optlist);
        s_optlist = NULL;
    }

    return 0;
}

static coap_al_ops_t s_libcoap_ops = {
    .init = __init,
    .destroy = __destroy,
    .add_option = __add_option,
    .request = __request,
    .send = __send,
    .recv = __recv,
};

int coap_setup(void)
{
    return coap_al_install(&s_libcoap_ops);
}
