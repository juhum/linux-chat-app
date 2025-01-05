#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define NAME_SIZE 50
#define MAX_GROUPS 10
#define GROUP_NAME_SIZE 50

typedef struct {
    char name[GROUP_NAME_SIZE];
    int members[MAX_CLIENTS];
} Group;

int client_sockets[MAX_CLIENTS];
char client_names[MAX_CLIENTS][NAME_SIZE];
Group groups[MAX_GROUPS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t groups_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message_to_all(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] != 0 && client_sockets[i] != sender_socket) {
            write(client_sockets[i], message, strlen(message));
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(char *message, char *recipient_name) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] != 0 && strcmp(client_names[i], recipient_name) == 0) {
            write(client_sockets[i], message, strlen(message));
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_group_message(char *message, char *group_name) {
    pthread_mutex_lock(&groups_mutex);
    for (int i = 0; i < MAX_GROUPS; ++i) {
        if (strcmp(groups[i].name, group_name) == 0) {
            for (int j = 0; j < MAX_CLIENTS; ++j) {
                if (groups[i].members[j] != 0) {
                    write(groups[i].members[j], message, strlen(message));
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(&groups_mutex);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Send welcome message with command info
    char welcome_message[] = "Welcome to the chat server!\n"
                             "Available commands:\n"
                             "/pm <name> <message> - Send a private message\n"
                             "/file <name> <file_path> - Send a file\n"
                             "/join <group_name> - Join a group\n"
                             "/group <group_name> <message> - Send a message to a group\n";
    write(client_socket, welcome_message, strlen(welcome_message));

    // Get client name
    bytes_read = read(client_socket, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    char client_name[NAME_SIZE];
    strncpy(client_name, buffer, NAME_SIZE - 1);
    client_name[NAME_SIZE - 1] = '\0';

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = client_socket;
            strncpy(client_names[i], client_name, NAME_SIZE - 1);
            client_names[i][NAME_SIZE - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received from %s: %s\n", client_name, buffer);

        // Check for private message
        if (strncmp(buffer, "/pm ", 4) == 0) {
            char *recipient_name = strtok(buffer + 4, " ");
            char *private_message = strtok(NULL, "\0");
            char formatted_message[BUFFER_SIZE];
            snprintf(formatted_message, sizeof(formatted_message), "[PM from %s] %s", client_name, private_message);
            send_private_message(formatted_message, recipient_name);
        } else if (strncmp(buffer, "/file ", 6) == 0) {
            char *recipient_name = strtok(buffer + 6, " ");
            char *file_content = strtok(NULL, "\0");
            char formatted_message[BUFFER_SIZE];
            snprintf(formatted_message, sizeof(formatted_message), "[File from %s] %s", client_name, file_content);
            send_private_message(formatted_message, recipient_name);
        } else if (strncmp(buffer, "/group ", 7) == 0) {
            char *group_name = strtok(buffer + 7, " ");
            char *group_message = strtok(NULL, "\0");
            char formatted_message[BUFFER_SIZE];
            snprintf(formatted_message, sizeof(formatted_message), "[Group %s from %s] %s", group_name, client_name, group_message);
            send_group_message(formatted_message, group_name);
        } else if (strncmp(buffer, "/join ", 6) == 0) {
            char *group_name = strtok(buffer + 6, " ");
            pthread_mutex_lock(&groups_mutex);
            int group_found = 0;
            for (int i = 0; i < MAX_GROUPS; ++i) {
                if (strcmp(groups[i].name, group_name) == 0) {
                    for (int j = 0; j < MAX_CLIENTS; ++j) {
                        if (groups[i].members[j] == 0) {
                            groups[i].members[j] = client_socket;
                            group_found = 1;
                            break;
                        }
                    }
                    break;
                }
            }
            if (!group_found) {
                for (int i = 0; i < MAX_GROUPS; ++i) {
                    if (groups[i].name[0] == '\0') {
                        strncpy(groups[i].name, group_name, GROUP_NAME_SIZE - 1);
                        groups[i].name[GROUP_NAME_SIZE - 1] = '\0';
                        groups[i].members[0] = client_socket;
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&groups_mutex);
        } else {
            // Broadcast message to all clients
            char formatted_message[BUFFER_SIZE];
            snprintf(formatted_message, sizeof(formatted_message), "[%s] %.*s", client_name, BUFFER_SIZE - (int)strlen(client_name) - 4, buffer);
            send_message_to_all(formatted_message, client_socket);
        }
    }

    close(client_socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(arg);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        int *new_sock = malloc(1);
        *new_sock = new_socket;
        pthread_create(&tid, NULL, handle_client, (void *)new_sock);
    }

    return 0;
}