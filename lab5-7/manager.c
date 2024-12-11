#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <signal.h>
#include <sys/wait.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

#define MAX_NODES 100

struct node_info {
    int id;
    pid_t pid;
    int parent;
    int children[2];
    int child_count;
    bool used;
    bool available;
};

static struct node_info nodes[MAX_NODES];

static const char *RABBIT_HOST = "localhost";
static int RABBIT_PORT = 5672;
static amqp_connection_state_t conn;
static amqp_channel_t channel = 1;

static int send_command_to_node(int node_id, const char *cmd, const char *corr_id) {
    char queue_name[64];
    snprintf(queue_name, sizeof(queue_name), "node_%d", node_id);

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CORRELATION_ID_FLAG;
    props.correlation_id = amqp_cstring_bytes(corr_id);

    amqp_basic_publish(conn, channel, amqp_empty_bytes, amqp_cstring_bytes(queue_name),
                       0, 0, &props, amqp_cstring_bytes(cmd));
    return 0;
}

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void initialize_nodes() {
    for (int i = 0; i < MAX_NODES; i++) {
        nodes[i].used = false;
        nodes[i].available = false;
        nodes[i].parent = -1;
        nodes[i].child_count = 0;
        nodes[i].children[0] = -1;
        nodes[i].children[1] = -1;
    }
}

static int find_free_node_index() {
    for (int i = 0; i < MAX_NODES; i++) {
        if (!nodes[i].used) {
            return i;
        }
    }
    return -1;
}

static int find_node_index_by_id(int id) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].id == id) {
            return i;
        }
    }
    return -1;
}

static void update_availability() {
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used) {
            if (kill(nodes[i].pid, 0) == -1) {
                nodes[i].available = false;
            } else {
                nodes[i].available = true;
            }
        }
    }
}

static void print_tree() {
    printf("Current nodes:\n");
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used) {
            printf("Node id=%d pid=%d parent=%d children=[",
                   nodes[i].id, nodes[i].pid, nodes[i].parent);
            for (int c = 0; c < nodes[i].child_count; c++) {
                printf("%d ", nodes[i].children[c]);
            }
            printf("] available=%d\n", nodes[i].available ? 1 : 0);
        }
    }
}

static int create_node(int id, int parent_id) {
    if (find_node_index_by_id(id) != -1) {
        printf("Error: Already exists\n");
        return -1;
    }

    int parent_index = -1;
    if (parent_id != -1) {
        parent_index = find_node_index_by_id(parent_id);
        if (parent_index == -1) {
            printf("Error: Parent not found\n");
            return -1;
        }
        if (!nodes[parent_index].available) {
            printf("Error: Parent is unavailable\n");
            return -1;
        }
        if (nodes[parent_index].child_count >= 2) {
            printf("Error: Parent node already has 2 children\n");
            return -1;
        }
    }

    int index = find_free_node_index();
    if (index == -1) {
        printf("Error: No space for more nodes\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printf("Error: Fork failed\n");
        return -1;
    } else if (pid == 0) {
        char node_id_str[10];
        snprintf(node_id_str, sizeof(node_id_str), "%d", id);
        execl("./node", "./node", node_id_str, NULL);
        _exit(1);
    }

    nodes[index].id = id;
    nodes[index].pid = pid;
    nodes[index].used = true;
    nodes[index].available = true;
    nodes[index].parent = parent_id;
    nodes[index].child_count = 0;

    if (parent_id != -1) {
        nodes[parent_index].children[nodes[parent_index].child_count++] = id;
    }

    printf("Ok: %d\n", pid);
    return 0;
}

static void exec_command(int id, const char *subcommand) {
    int idx = find_node_index_by_id(id);
    if (idx == -1) {
        printf("Error:%d: Not found\n", id);
        return;
    }
    if (!nodes[idx].available) {
        printf("Error:%d: Node is unavailable\n", id);
        return;
    }

    uuid_t uuid;
    uuid_generate(uuid);
    char corr_id[37];
    uuid_unparse_lower(uuid, corr_id);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "exec %s", subcommand);

    send_command_to_node(id, cmd, corr_id);
    printf("Command sent to node %d: %s\n", id, subcommand);
}

static void ping_command(int id) {
    int idx = find_node_index_by_id(id);
    if (idx == -1) {
        printf("Error: Not found\n");
        return;
    }

    if (kill(nodes[idx].pid, 0) == -1) {
        printf("Ok: 0\n");
        return;
    }
    printf("Ok: 1\n");
}

static void setup_rabbitmq() {
    conn = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        die("Creating TCP socket failed");
    }

    if (amqp_socket_open(socket, RABBIT_HOST, RABBIT_PORT)) {
        die("Opening TCP socket failed");
    }

    amqp_rpc_reply_t r = amqp_login(conn, "/", 0, 131072, 0,
                                    AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        die("Login failed");
    }

    amqp_channel_open(conn, channel);
    r = amqp_get_rpc_reply(conn);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        die("Opening channel failed");
    }
    amqp_queue_purge(conn, channel, amqp_cstring_bytes("manager_response"));

    amqp_queue_declare(conn, channel, amqp_cstring_bytes("manager_response"), 0, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(conn);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        die("Queue declare failed");
    }

    amqp_basic_consume(conn, channel, amqp_cstring_bytes("manager_response"), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(conn);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) {
        die("Consume on manager_response failed");
    }
}
static void *response_thread(void *arg) {
    (void)arg;
    while (1) {
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(conn);
        amqp_rpc_reply_t res = amqp_consume_message(conn, &envelope, NULL, 0);
        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            printf("%.*s\n", (int)envelope.message.body.len, (char *)envelope.message.body.bytes);
            fflush(stdout);

            amqp_destroy_envelope(&envelope);
        } else {
            printf("Error: amqp_consume_message failed: %s\n", amqp_error_string2(res.reply_type));
            break;
        }
    }
    return NULL;
}


static void handle_command(char *line) {
    char *parts[4];
    int count = 0;

    char *token = strtok(line, " \t\n");
    while (token && count < 4) {
        parts[count++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (count == 0) return;

    if (strcmp(parts[0], "create") == 0) {
        if (count < 2) {
            printf("Error: Missing ID\n");
            return;
        }
        int id = atoi(parts[1]);
        int parent_id = -1;
        if (count == 3) {
            parent_id = atoi(parts[2]);
        }
        create_node(id, parent_id);
    } else if (strcmp(parts[0], "exec") == 0) {
        if (count < 3) {
            printf("Error: Missing exec command\n");
            return;
        }
        int id = atoi(parts[1]);
        exec_command(id, parts[2]);
    } else if (strcmp(parts[0], "ping") == 0) {
        if (count < 2) {
            printf("Error: Missing ID\n");
            return;
        }
        int id = atoi(parts[1]);
        ping_command(id);
    } else if (strcmp(parts[0], "list") == 0) {
        update_availability();
        print_tree();
    } else if (strcmp(parts[0], "exit") == 0) {
        printf("Exiting manager...\n");
        exit(0);
    } else {
        printf("Unknown command\n");
    }
}

static void sigchld_handler(int signo) {
    (void)signo;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_NODES; i++) {
            if (nodes[i].used && nodes[i].pid == pid) {
                nodes[i].available = false;
                break;
            }
        }
    }
}

int main() {
    initialize_nodes();
    setup_rabbitmq();

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    pthread_t thr;
    pthread_create(&thr, NULL, response_thread, NULL);
    pthread_detach(thr);

    printf("Manager started. Waiting for commands...\n");
    char line[256];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        handle_command(line);
    }

    return 0;
}
