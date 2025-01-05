# Linux Chat Application

This is a simple chat application written in C for Linux. It supports multiple clients, reconnections, private chats, group chats, and file sharing.

## Features

- **Multiple Clients**: Supports at least 3 clients.
- **Reconnections**: Clients can reconnect to the server.
- **Private Chats**: Send private messages to specific clients.
- **Group Chats**: Create and join groups to chat with multiple clients.
- **File Sharing**: Send files to other clients.

## Commands

- `/pm <name> <message>`: Send a private message to a client.
- `/file <name> <file_path>`: Send a file to a client.
- `/join <group_name>`: Join a group.
- `/group <group_name> <message>`: Send a message to a group.

## Getting Started

### Prerequisites

- GCC (GNU Compiler Collection)
- Make

### Installation

1. Clone the repository:

    ```sh
    git clone https://github.com/yourusername/linux-chat-app.git
    cd linux-chat-app
    ```

2. Compile the code:

    ```sh
    gcc -o server server.c -lpthread
    gcc -o client client.c -lpthread
    ```

### Running the Server

1. Open a terminal and navigate to the project directory.
2. Run the server:

    ```sh
    ./server
    ```

### Running the Client

1. Open a terminal and navigate to the project directory.
2. Run the client:

    ```sh
    ./client
    ```

3. Enter your name when prompted.

### Testing the Application

1. Open multiple terminal windows to simulate different clients.
2. Run the client in each terminal and enter unique names.
3. Use the available commands to test private messages, group chats, and file sharing.

### Example Usage

- **Broadcast message**: Type a message and press Enter.
- **Private message**: `/pm Bob Hi Bob, this is a private message.`
- **File sharing**: `/file Bob /path/to/file.txt`
- **Join group**: `/join group1`
- **Group message**: `/group group1 Hello group!`
