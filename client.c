#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *receive_messages(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(sock, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }

    return NULL;
}

void send_file(int sock, char *recipient_name, char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("File open failed");
        return;
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "/file %s ", recipient_name);
    int header_len = strlen(buffer);
    int bytes_read;

    while ((bytes_read = fread(buffer + header_len, 1, BUFFER_SIZE - header_len, file)) > 0) {
        write(sock, buffer, header_len + bytes_read);
        header_len = 0; // Only include the header in the first packet
    }

    fclose(file);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];
    pthread_t tid;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Send client name
    printf("Enter your name: ");
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = '\0';
    write(sock, message, strlen(message));

    pthread_create(&tid, NULL, receive_messages, (void *)&sock);

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0';

        if (strncmp(message, "/file ", 6) == 0) {
            char *recipient_name = strtok(message + 6, " ");
            char *file_path = strtok(NULL, "\0");
            send_file(sock, recipient_name, file_path);
        } else {
            write(sock, message, strlen(message));
        }
    }

    close(sock);
    return 0;
}
