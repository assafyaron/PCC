#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#define BUFFER_SIZE 1024*1024 // 1MB buffer size
#define FAILURE 1
#define SUCCESS 0

// Struct to count how many times each printable character was observed in all client connections
typedef struct {
    uint32_t printable_chars[95]; // 95 printable characters
} pcc_total;

/* ==== Global variables ==== */
pcc_total* pcc_total_global;
int server_on = 1; // Flag to indicate if server is on
/* ========================== */

// SIGINT handler
void sigint_handler(int signum) {
    server_on = 0;
}

uint32_t count_printable_chars(const char* buffer, ssize_t bytes_read, pcc_total* pcc_total_local) {
    /*
    Function to count how many printable characters are in a buffer.
    Args:
        buffer: buffer to count printable characters from
        bytes_read: number of bytes read from buffer
        pcc_total_local: local printable characters count
    Returns:
        printable_chars_cnt: number of printable characters in buffer
    */
    ssize_t i;
    uint32_t printable_chars_cnt = 0;
    for (i=0;i<bytes_read;i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
            printable_chars_cnt++;
            pcc_total_local->printable_chars[buffer[i]-32]++;}}
    return printable_chars_cnt;
}

uint32_t process_connection(int accept_sockfd, pcc_total* pcc_total_local, uint32_t file_size) {
    /*
    Function to process a connection from a client - read data from client and count printable characters.
    Args:
        accept_sockfd: socket file descriptor to accept connection
        pcc_total_local: local printable characters count
        file_size: size of file to read from client
    Returns:
        printable_chars_cnt: number of printable characters in file
    */
    ssize_t total_bytes_read = 0;
    ssize_t bytes_read = 0;
    uint32_t printable_chars_cnt = 0;
    char buffer[BUFFER_SIZE];

    // Read data from client
    while ((bytes_read = read(accept_sockfd, buffer, BUFFER_SIZE)) > 0) {
        // Count printable characters
        printable_chars_cnt += count_printable_chars(buffer, bytes_read, pcc_total_local);
        total_bytes_read += bytes_read;
        if (total_bytes_read >= file_size) {
            break;}
    }
    if (bytes_read == 0) { // End of file or client closed connection
        perror("Error reading data from client, client proccess terminated or killed");}
    else if (bytes_read < 0 && ((errno == ETIMEDOUT) || (errno == ECONNRESET) || (errno == EPIPE))) {
        perror("Error reading data from client");
    }
    return printable_chars_cnt;
}

int main(int argc, char* argv[]) {

    /* ===== STEP 0: Declare variables, extract arguments, define signal handler and init structs ===== */

    struct sockaddr_in server_addr;
    pcc_total* pcc_total_local;
    uint32_t file_size;
    uint32_t printable_chars_cnt;    
    uint16_t server_port;
    int listen_sockfd;
    int accept_sockfd;
    int i;
    const int allow = 1;

    // Expect argument format: <./pcc_server> <server_port>
    if (argc != 2) {
        perror("Invalid number of arguments. Expected 1 argument");
        exit(FAILURE);}

    // Set new handler for SIGINT
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("Error setting new handler for SIGINT");
        exit(FAILURE);}

    // Extract arguments and init pcc_total_local and pcc_total_global 
    server_port = atoi(argv[1]);

    // Init pcc_total_local and pcc_total_global
    pcc_total_local = (pcc_total*)malloc(sizeof(pcc_total));
    if (pcc_total_local == NULL) {
        perror("Error allocating memory for pcc_total_local");
        exit(FAILURE);}
    pcc_total_global = (pcc_total*)malloc(sizeof(pcc_total));
    if (pcc_total_global == NULL) {
        perror("Error allocating memory for pcc_total_global");
        exit(FAILURE);}

    for (i=0;i<95;i++) {
        pcc_total_local->printable_chars[i] = 0;
        pcc_total_global->printable_chars[i] = 0;}
    
    /* ===== STEP 1: Listen for incoming connections on the server port ===== */

    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (listen_sockfd < 0) {
        perror("Error creating socket");
        exit(FAILURE);}

    // Set socket option SO_REUSEADDR to allow binding to the same port in case of TIME_WAITS
    if (setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &allow, sizeof(allow)) < 0) {
        perror("Error setting socket option SO_REUSEADDR"); 
        exit(FAILURE);}

    // Set up server address
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to server address
    if (bind(listen_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        perror("Error binding socket to server address");
        exit(FAILURE);}

    // Listen for incoming connections
    if (listen(listen_sockfd, 10) != 0) {
        perror("Error listening for incoming connections");
        exit(FAILURE);}

    /* ===== STEP 2: Accept incoming connections and handle them ===== */

    while (server_on == 1) {

        // Accept incoming connection
        accept_sockfd = accept(listen_sockfd, NULL,NULL);
        if (accept_sockfd < 0) {
            perror("Error accepting incoming connection");
        }

        // Read first 4 bytes to get file size
        if (read(accept_sockfd, &file_size, sizeof(file_size)) != sizeof(file_size)) {
            perror("Error reading file size from client");
        }

        // Process incoming connection
        if (file_size > 0) {
            printable_chars_cnt = process_connection(accept_sockfd, pcc_total_local, ntohl(file_size)); 
        }

        // Send printable characters count to client
        printable_chars_cnt = htonl(printable_chars_cnt);
        if (write(accept_sockfd, &printable_chars_cnt, sizeof(printable_chars_cnt)) != sizeof(printable_chars_cnt)) {
            perror("Error sending printable characters count to client");
        }

        // Update global\local printable characters count
        for (i=0;i<95;i++) {
            pcc_total_global->printable_chars[i] += pcc_total_local->printable_chars[i];
            pcc_total_local->printable_chars[i] = 0;
            printable_chars_cnt = 0;}

        // Close connection
        close(accept_sockfd);
    }

    /* ===== STEP 3: Handle SIGINT - Print total printable characters count and exit ===== */

    if (server_on == 0) {
        for (i=0;i<95;i++) {
            printf("char '%c' : %u times\n", (char)(i+32), pcc_total_global->printable_chars[i]);}
    }

    exit(SUCCESS); // Exit successfully
}