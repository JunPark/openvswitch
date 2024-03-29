/*
 * Copyright (c) 2009, 2010, 2011 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>
#include "vconn.h"
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "command-line.h"
#include "openflow/openflow.h"
#include "poll-loop.h"
#include "socket-util.h"
#include "stream.h"
#include "stream-ssl.h"
#include "timeval.h"
#include "util.h"
#include "vlog.h"

#undef NDEBUG
#include <assert.h>

struct fake_pvconn {
    const char *type;
    char *pvconn_name;
    char *vconn_name;
    struct pstream *pstream;
};

static void
check(int a, int b, const char *as, const char *file, int line)
{
    if (a != b) {
        ovs_fatal(0, "%s:%d: %s is %d but should be %d", file, line, as, a, b);
    }
}


#define CHECK(A, B) check(A, B, #A, __FILE__, __LINE__)

static void
check_errno(int a, int b, const char *as, const char *file, int line)
{
    if (a != b) {
        char *str_b = strdup(strerror(abs(b)));
        ovs_fatal(0, "%s:%d: %s is %d (%s) but should be %d (%s)",
                  file, line, as, a, strerror(abs(a)), b, str_b);
    }
}

#define CHECK_ERRNO(A, B) check_errno(A, B, #A, __FILE__, __LINE__)

static void
fpv_create(const char *type, struct fake_pvconn *fpv)
{
#ifdef HAVE_OPENSSL
    if (!strcmp(type, "ssl")) {
        stream_ssl_set_private_key_file("testpki-privkey.pem");
        stream_ssl_set_certificate_file("testpki-cert.pem");
        stream_ssl_set_ca_cert_file("testpki-cacert.pem", false);
    }
#endif

    fpv->type = type;
    if (!strcmp(type, "unix")) {
        static int unix_count = 0;
        char *bind_path;

        bind_path = xasprintf("fake-pvconn.%d", unix_count++);
        fpv->pvconn_name = xasprintf("punix:%s", bind_path);
        fpv->vconn_name = xasprintf("unix:%s", bind_path);
        CHECK_ERRNO(pstream_open(fpv->pvconn_name, &fpv->pstream,
                                 DSCP_DEFAULT), 0);
        free(bind_path);
    } else if (!strcmp(type, "tcp") || !strcmp(type, "ssl")) {
        char *s, *port, *save_ptr = NULL;
        char *open_name;

        open_name = xasprintf("p%s:0:127.0.0.1", type);
        CHECK_ERRNO(pstream_open(open_name, &fpv->pstream, DSCP_DEFAULT), 0);

        /* Extract bound port number from pstream name. */
        s = xstrdup(pstream_get_name(fpv->pstream));
        strtok_r(s, ":", &save_ptr);
        port = strtok_r(NULL, ":", &save_ptr);

        /* Save info. */
        fpv->pvconn_name = xstrdup(pstream_get_name(fpv->pstream));
        fpv->vconn_name = xasprintf("%s:127.0.0.1:%s", type, port);

        free(open_name);
        free(s);
    } else {
        abort();
    }
}

static struct stream *
fpv_accept(struct fake_pvconn *fpv)
{
    struct stream *stream;

    CHECK_ERRNO(pstream_accept_block(fpv->pstream, &stream), 0);

    return stream;
}

static void
fpv_close(struct fake_pvconn *fpv)
{
    pstream_close(fpv->pstream);
    fpv->pstream = NULL;
}

static void
fpv_destroy(struct fake_pvconn *fpv)
{
    fpv_close(fpv);
    free(fpv->pvconn_name);
    free(fpv->vconn_name);
}

/* Connects to a fake_pvconn with vconn_open(), then closes the listener and
 * verifies that vconn_connect() reports 'expected_error'. */
static void
test_refuse_connection(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    int expected_error;
    struct fake_pvconn fpv;
    struct vconn *vconn;

    expected_error = (!strcmp(type, "unix") ? EPIPE
                      : !strcmp(type, "tcp") ? ECONNRESET
                      : EPROTO);

    fpv_create(type, &fpv);
    CHECK_ERRNO(vconn_open(fpv.vconn_name, OFP10_VERSION, &vconn,
                           DSCP_DEFAULT), 0);
    fpv_close(&fpv);
    vconn_run(vconn);
    CHECK_ERRNO(vconn_connect(vconn), expected_error);
    vconn_close(vconn);
    fpv_destroy(&fpv);
}

/* Connects to a fake_pvconn with vconn_open(), accepts that connection and
 * closes it immediately, and verifies that vconn_connect() reports
 * 'expected_error'. */
static void
test_accept_then_close(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    int expected_error;
    struct fake_pvconn fpv;
    struct vconn *vconn;

    expected_error = (!strcmp(type, "unix") ? EPIPE
                      : !strcmp(type, "tcp") ? ECONNRESET
                      : EPROTO);

    fpv_create(type, &fpv);
    CHECK_ERRNO(vconn_open(fpv.vconn_name, OFP10_VERSION, &vconn,
                           DSCP_DEFAULT), 0);
    vconn_run(vconn);
    stream_close(fpv_accept(&fpv));
    fpv_close(&fpv);
    CHECK_ERRNO(vconn_connect(vconn), expected_error);
    vconn_close(vconn);
    fpv_destroy(&fpv);
}

/* Connects to a fake_pvconn with vconn_open(), accepts that connection and
 * reads the hello message from it, then closes the connection and verifies
 * that vconn_connect() reports 'expected_error'. */
static void
test_read_hello(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    struct fake_pvconn fpv;
    struct vconn *vconn;
    struct stream *stream;

    fpv_create(type, &fpv);
    CHECK_ERRNO(vconn_open(fpv.vconn_name, OFP10_VERSION, &vconn,
                           DSCP_DEFAULT), 0);
    vconn_run(vconn);
    stream = fpv_accept(&fpv);
    fpv_destroy(&fpv);
    for (;;) {
       struct ofp_header hello;
       int retval;

       retval = stream_recv(stream, &hello, sizeof hello);
       if (retval == sizeof hello) {
           CHECK(hello.version, OFP10_VERSION);
           CHECK(hello.type, OFPT_HELLO);
           CHECK(ntohs(hello.length), sizeof hello);
           break;
       } else {
           CHECK_ERRNO(retval, -EAGAIN);
       }

       vconn_run(vconn);
       CHECK_ERRNO(vconn_connect(vconn), EAGAIN);
       vconn_run_wait(vconn);
       vconn_connect_wait(vconn);
       stream_recv_wait(stream);
       poll_block();
    }
    stream_close(stream);
    CHECK_ERRNO(vconn_connect(vconn), ECONNRESET);
    vconn_close(vconn);
}

/* Connects to a fake_pvconn with vconn_open(), accepts that connection and
 * sends the 'out' bytes in 'out_size' to it (presumably an OFPT_HELLO
 * message), then verifies that vconn_connect() reports
 * 'expect_connect_error'. */
static void
test_send_hello(const char *type, const void *out, size_t out_size,
                int expect_connect_error)
{
    struct fake_pvconn fpv;
    struct vconn *vconn;
    bool read_hello, connected;
    struct ofpbuf *msg;
    struct stream *stream;
    size_t n_sent;

    fpv_create(type, &fpv);
    CHECK_ERRNO(vconn_open(fpv.vconn_name, OFP10_VERSION, &vconn,
                           DSCP_DEFAULT), 0);
    vconn_run(vconn);
    stream = fpv_accept(&fpv);
    fpv_destroy(&fpv);

    n_sent = 0;
    while (n_sent < out_size) {
        int retval;

        retval = stream_send(stream, (char *) out + n_sent, out_size - n_sent);
        if (retval > 0) {
            n_sent += retval;
        } else if (retval == -EAGAIN) {
            stream_run(stream);
            vconn_run(vconn);
            stream_recv_wait(stream);
            vconn_connect_wait(vconn);
            vconn_run_wait(vconn);
            poll_block();
        } else {
            ovs_fatal(0, "stream_send returned unexpected value %d", retval);
        }
    }

    read_hello = connected = false;
    for (;;) {
       if (!read_hello) {
           struct ofp_header hello;
           int retval = stream_recv(stream, &hello, sizeof hello);
           if (retval == sizeof hello) {
               CHECK(hello.version, OFP10_VERSION);
               CHECK(hello.type, OFPT_HELLO);
               CHECK(ntohs(hello.length), sizeof hello);
               read_hello = true;
           } else {
               CHECK_ERRNO(retval, -EAGAIN);
           }
       }

       vconn_run(vconn);
       if (!connected) {
           int error = vconn_connect(vconn);
           if (error == expect_connect_error) {
               if (!error) {
                   connected = true;
               } else {
                   stream_close(stream);
                   vconn_close(vconn);
                   return;
               }
           } else {
               CHECK_ERRNO(error, EAGAIN);
           }
       }

       if (read_hello && connected) {
           break;
       }

       vconn_run_wait(vconn);
       if (!connected) {
           vconn_connect_wait(vconn);
       }
       if (!read_hello) {
           stream_recv_wait(stream);
       }
       poll_block();
    }
    stream_close(stream);
    CHECK_ERRNO(vconn_recv(vconn, &msg), EOF);
    vconn_close(vconn);
}

/* Try connecting and sending a normal hello, which should succeed. */
static void
test_send_plain_hello(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    struct ofp_header hello;

    hello.version = OFP10_VERSION;
    hello.type = OFPT_HELLO;
    hello.length = htons(sizeof hello);
    hello.xid = htonl(0x12345678);
    test_send_hello(type, &hello, sizeof hello, 0);
}

/* Try connecting and sending an extra-long hello, which should succeed (since
 * the specification says that implementations must accept and ignore extra
 * data). */
static void
test_send_long_hello(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    struct ofp_header hello;
    char buffer[sizeof hello * 2];

    hello.version = OFP10_VERSION;
    hello.type = OFPT_HELLO;
    hello.length = htons(sizeof buffer);
    hello.xid = htonl(0x12345678);
    memset(buffer, 0, sizeof buffer);
    memcpy(buffer, &hello, sizeof hello);
    test_send_hello(type, buffer, sizeof buffer, 0);
}

/* Try connecting and sending an echo request instead of a hello, which should
 * fail with EPROTO. */
static void
test_send_echo_hello(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    struct ofp_header echo;

    echo.version = OFP10_VERSION;
    echo.type = OFPT_ECHO_REQUEST;
    echo.length = htons(sizeof echo);
    echo.xid = htonl(0x89abcdef);
    test_send_hello(type, &echo, sizeof echo, EPROTO);
}

/* Try connecting and sending a hello packet that has its length field as 0,
 * which should fail with EPROTO. */
static void
test_send_short_hello(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    struct ofp_header hello;

    memset(&hello, 0, sizeof hello);
    test_send_hello(type, &hello, sizeof hello, EPROTO);
}

/* Try connecting and sending a hello packet that has a bad version, which
 * should fail with EPROTO. */
static void
test_send_invalid_version_hello(int argc OVS_UNUSED, char *argv[])
{
    const char *type = argv[1];
    struct ofp_header hello;

    hello.version = OFP10_VERSION - 1;
    hello.type = OFPT_HELLO;
    hello.length = htons(sizeof hello);
    hello.xid = htonl(0x12345678);
    test_send_hello(type, &hello, sizeof hello, EPROTO);
}

static const struct command commands[] = {
    {"refuse-connection", 1, 1, test_refuse_connection},
    {"accept-then-close", 1, 1, test_accept_then_close},
    {"read-hello", 1, 1, test_read_hello},
    {"send-plain-hello", 1, 1, test_send_plain_hello},
    {"send-long-hello", 1, 1, test_send_long_hello},
    {"send-echo-hello", 1, 1, test_send_echo_hello},
    {"send-short-hello", 1, 1, test_send_short_hello},
    {"send-invalid-version-hello", 1, 1, test_send_invalid_version_hello},
    {NULL, 0, 0, NULL},
};

int
main(int argc, char *argv[])
{
    set_program_name(argv[0]);
    vlog_set_levels(NULL, VLF_ANY_FACILITY, VLL_EMER);
    vlog_set_levels(NULL, VLF_CONSOLE, VLL_DBG);
    signal(SIGPIPE, SIG_IGN);

    time_alarm(10);

    run_command(argc - 1, argv + 1, commands);

    return 0;
}
