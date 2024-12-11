#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

static int node_id = -1;
static bool running = false;
static double accumulated = 0.0;
static double start_time = 0.0;

static const char *RABBIT_HOST = "localhost";
static int RABBIT_PORT = 5672;

static double now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void timer_start() {
    if (!running) {
        running = true;
        start_time = now_ms();
    }
}

static void timer_stop() {
    if (running) {
        accumulated += now_ms() - start_time;
        running = false;
    }
}

static int timer_get_ms() {
    double total = accumulated;
    if (running) {
        total += now_ms() - start_time;
    }
    return (int)(total * 1000);
}

static amqp_connection_state_t conn;
static amqp_channel_t channel = 1;

static void die_amqp_rpc_reply(const char *context, amqp_rpc_reply_t r) {
    switch (r.reply_type) {
        case AMQP_RESPONSE_NORMAL:
            return;

        case AMQP_RESPONSE_NONE:
            fprintf(stderr, "%s: missing RPC reply type!\n", context);
            break;

        case AMQP_RESPONSE_LIBRARY_EXCEPTION:
            fprintf(stderr, "%s: %s\n", context, amqp_error_string2(r.library_error));
            break;

        case AMQP_RESPONSE_SERVER_EXCEPTION:
            if (r.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
                amqp_connection_close_t *m = (amqp_connection_close_t *) r.reply.decoded;
                fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
                        context, m->reply_code, (int)m->reply_text.len, (char *)m->reply_text.bytes);
            } else if (r.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
                amqp_channel_close_t *m = (amqp_channel_close_t *) r.reply.decoded;
                fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
                        context, m->reply_code, (int)m->reply_text.len, (char *)m->reply_text.bytes);
            } else {
                fprintf(stderr, "%s: unknown server error, method id 0x%08X\n", context, r.reply.id);
            }
            break;
    }
    exit(1);
}

static void setup_rabbitmq() {
    conn = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        fprintf(stderr, "Creating TCP socket failed\n");
        exit(1);
    }

    if (amqp_socket_open(socket, RABBIT_HOST, RABBIT_PORT)) {
        fprintf(stderr, "Opening TCP socket failed\n");
        exit(1);
    }

    amqp_rpc_reply_t r = amqp_login(conn, "/", 0, 131072, 0,
                                    AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    die_amqp_rpc_reply("Login", r);

    amqp_channel_open(conn, channel);
    r = amqp_get_rpc_reply(conn);
    die_amqp_rpc_reply("Opening channel", r);

    char queue_name[64];
    snprintf(queue_name, sizeof(queue_name), "node_%d", node_id);

    amqp_queue_declare(conn, channel, amqp_cstring_bytes(queue_name), 0, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(conn);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        die_amqp_rpc_reply("Queue declare failed", r);
    }

    amqp_basic_consume(conn, channel, amqp_cstring_bytes(queue_name), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(conn);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        die_amqp_rpc_reply("Consume failed", r);
    }
}

static void send_response(const char *correlation_id, const char *body) {
    amqp_basic_properties_t props;
    memset(&props, 0, sizeof(props));
    props._flags = AMQP_BASIC_CORRELATION_ID_FLAG;
    props.correlation_id = amqp_cstring_bytes(correlation_id);

    int res = amqp_basic_publish(conn, channel, amqp_empty_bytes, amqp_cstring_bytes("manager_response"),
                       0, 0, &props, amqp_cstring_bytes(body));
    if (res < 0) {
        fprintf(stderr, "Failed to publish response: %s\n", body);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <node_id>\n", argv[0]);
        return 1;
    }

    node_id = atoi(argv[1]);
    printf("Node %d started, pid=%d\n", node_id, getpid());
    fflush(stdout);

    setup_rabbitmq();

    while (1) {
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(conn);
        amqp_rpc_reply_t res = amqp_consume_message(conn, &envelope, NULL, 0);

        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            if (envelope.message.properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
                char corr_id[256];
                snprintf(corr_id, sizeof(corr_id), "%.*s",
                         (int)envelope.message.properties.correlation_id.len,
                         (char *)envelope.message.properties.correlation_id.bytes);

                char cmd[256];
                snprintf(cmd, sizeof(cmd), "%.*s", (int)envelope.message.body.len, (char *)envelope.message.body.bytes);

                if (strncmp(cmd, "exec", 4) == 0) {
                    char *sub = strchr(cmd, ' ');
                    if (sub) {
                        sub++;
                        if (strcmp(sub, "start") == 0) {
                            timer_start();
                            char resp[64];
                            snprintf(resp, sizeof(resp), "Ok:%d", node_id);
                            send_response(corr_id, resp);
                        } else if (strcmp(sub, "stop") == 0) {
                            timer_stop();
                            char resp[64];
                            snprintf(resp, sizeof(resp), "Ok:%d", node_id);
                            send_response(corr_id, resp);
                        } else if (strcmp(sub, "time") == 0) {
                            int ms = timer_get_ms();
                            char resp[64];
                            snprintf(resp, sizeof(resp), "Ok:%d: %d", node_id, ms);
                            send_response(corr_id, resp);
                        } else {
                            char resp[64];
                            snprintf(resp, sizeof(resp), "Error:%d: Unknown exec command", node_id);
                            send_response(corr_id, resp);
                        }
                    } else {
                        char resp[64];
                        snprintf(resp, sizeof(resp), "Error:%d: Missing subcommand", node_id);
                        send_response(corr_id, resp);
                    }
                } else if (strncmp(cmd, "ping", 4) == 0) {
                    char resp[64];
                    snprintf(resp, sizeof(resp), "Ok:%d:1", node_id);
                    send_response(corr_id, resp);
                } else {
                    char resp[64];
                    snprintf(resp, sizeof(resp), "Error:%d: Unknown command", node_id);
                    send_response(corr_id, resp);
                }
            }

            amqp_destroy_envelope(&envelope);
        } else {
            fprintf(stderr, "Error: amqp_consume_message failed\n");
            break;
        }
    }

    amqp_channel_close(conn, channel, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);

    return 0;
}
