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

int main(int argc, char *argv[]) {
    
    /* ===== STEP 0: Declare variables and extract arguments ===== */

    struct in_addr server_ip;
    struct sockaddr_in server_addr;
    char file_data_buffer[BUFFER_SIZE];
    char* path_to_file;
    int fd;
    int sockfd;
    ssize_t bytes_read;
    ssize_t bytes_written;
    uint32_t file_size;
    uint32_t network_file_size;
    uint32_t network_printable_chars;
    uint32_t printable_chars;
    uint16_t server_port;

    // Expect argument format: <./pcc_client> <server_ip> <server_port> <path_to_file>
    if (argc != 4) {
        perror("Invalid number of arguments. Expected 3 arguments");
        exit(FAILURE);}

    // Extract arguments
    if (inet_pton(AF_INET, argv[1], &server_ip) != 1) {
        perror("Invalid server IP address");
        exit(FAILURE);}
    server_port = atoi(argv[2]);
    path_to_file = argv[3];

    /* ===== STEP 1: Open file and get size using syscalls ===== */

    fd = open(path_to_file, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(FAILURE);}

    file_size = lseek(fd, 0, SEEK_END); 
    if (file_size == (uint32_t)-1) {
        perror("Error getting file size");
        exit(FAILURE);}

    // Reset file pointer to beginning
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("Error resetting file pointer");
        exit(FAILURE);}

    /* ===== STEP 2: Create a TCP connection to the server port on the server IP ===== */

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(FAILURE);}

    // Set up server address
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr = server_ip;

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        exit(FAILURE);}

    /* ===== STEP 3: Send file size to server ===== */

    network_file_size = htonl(file_size);
    if (write(sockfd, &network_file_size, sizeof(network_file_size)) != sizeof(network_file_size)) {
        perror("Error sending file size to server");
        exit(FAILURE);}

    /* ===== STEP 4: Send file contents to server ===== */

    while ((bytes_read = read(fd, file_data_buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(sockfd, file_data_buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Error sending file contents to server");
            exit(FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("Error reading file");
        exit(FAILURE);}
    
    /* ===== STEP 5: Receive and print number of printable characters from server ===== */

    if (read(sockfd, &network_printable_chars, sizeof(network_printable_chars)) != sizeof(network_printable_chars)) {
        perror("Error receiving number of printable characters from server");
        exit(FAILURE);}
    printable_chars = ntohl(network_printable_chars);
    printf("# of printable characters: %u\n", printable_chars);

    exit(SUCCESS); // Exit successfully
}