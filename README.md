# PCC: Parallel Character Counter
This project implements a parallel client-server application for counting character occurrences in text. The server receives data from multiple clients and efficiently processes the text using multithreading. The results are then aggregated and returned to the respective clients. This implementation is written in C and follows a parallel processing approach to enhance performance.

## Files
- `pcc_client.c` - The client program that reads input from the user or a file and sends text data to the server.
- `pcc_server.c` - The server program that listens for incoming client connections, processes the received text, counts character occurrences, and sends back the results.
- `pcc_test.c` - A test program designed to validate the correctness and efficiency of the client-server implementation.

## Features
- **Client-server architecture:** Uses sockets to establish communication between clients and the server.
- **Multithreading:** The server efficiently handles multiple client requests in parallel using the POSIX threading library (`pthread`).
- **Dynamic Memory Management:** Handles large text inputs efficiently.
- **Robust Error Handling:** Includes mechanisms to handle connection failures, invalid inputs, and resource cleanup.
- **Scalability:** Can support multiple concurrent client connections.

## Compilation
To compile the project, run:
```sh
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c -o pcc_server
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_client.c -o pcc_client
```

## Usage
### Start the server in the background:
```sh
./pcc_server <port> &
```
- `<port>`: The port number on which the server listens for incoming connections.

### Start a client:
```sh
./pcc_client <server_address> <port>
```
- `<server_address>`: The IP address or hostname of the server.
- `<port>`: The port number the server is listening on.

After starting the client, enter a string of text. The client sends the text to the server, which processes it and returns the character counts.

## Example Run
### Server:
```sh
$ ./pcc_server 8080 &
Server started, listening on port 8080...
```

### Client:
```sh
$ ./pcc_client 127.0.0.1 8080
Enter text: hello world
Character count received from server:
'h': 1
'e': 1
'l': 3
'o': 2
'w': 1
'r': 1
'd': 1
```

## Testing
To verify the correctness of the implementation, compile and run the test program:
```sh
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_test.c -o pcc_test
./pcc_test
```
The test program will simulate multiple clients sending text data and verify that the server processes the data correctly.
