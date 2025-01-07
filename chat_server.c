/*
 * tcpserver.c - A simple TCP echo server
 * usage: tcpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"HTTPRequest.h"
#include "cJSON.h"

#define BUFSIZE 1024

#if 0
/*
 * Structs exported from in.h
 */

/* Internet address */
struct in_addr {
    unsigned int s_addr;
};

/* Internet style socket address */
struct sockaddr_in  {
    unsigned short int sin_family; /* Address family */
    unsigned short int sin_port;   /* Port number */
    struct in_addr sin_addr;	 /* IP address */
    unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
    char    *h_name;        /* official name of host */
    char    **h_aliases;    /* alias list */
    int     h_addrtype;     /* host address type */
    int     h_length;       /* length of address */
    char    **h_addr_list;  /* list of addresses */
}
#endif

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(1);
}

void send_json_response(int client_socket) {

    // Example JSON data to return

    const char *json_data = "{\"message\": \"Hello from C Backend!\", \"status\": \"success\"}";

    // HTTP Response headers
    const char *http_header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n";  // Blank line between headers and body

    // Get the length of the JSON data
    int json_data_length = strlen(json_data);

    // Send the HTTP header
    char header_buffer[256];
    snprintf(header_buffer, sizeof(header_buffer), http_header, json_data_length);
    send(client_socket, header_buffer, strlen(header_buffer), 0);

    // Send the JSON data
    send(client_socket, json_data, json_data_length, 0);

}

void send_connected_info(int client_socket, char *buff) {

    // Example JSON data to return

    printf("buffer data is %s\n", buff);

    cJSON *parsed_json = cJSON_Parse(buff);
    cJSON *data_item = cJSON_GetObjectItem(parsed_json, "data");
    if (cJSON_IsString(data_item) && (data_item->valuestring != NULL)) {
        printf("Extracted data: %s\n", data_item->valuestring);
    } else {
        printf("Error: 'data' field is missing or not a string\n");
    }

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "name", data_item->valuestring);

    char *json_string = cJSON_PrintUnformatted(response);

    // Free the cJSON object (not the string)
    cJSON_Delete(response);

    // HTTP Response headers
    const char *http_header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n";  // Blank line between headers and body

    // Get the length of the JSON data
    int json_data_length = strlen(json_string);

    // Send the HTTP header
    char header_buffer[256];
    snprintf(header_buffer, sizeof(header_buffer), http_header, json_data_length);
    send(client_socket, header_buffer, strlen(header_buffer), 0);

    // Send the JSON data
    send(client_socket, json_string, json_data_length, 0);

}


void send_html_response(int client_socket, char *html_file) {
    FILE *file = fopen(html_file, "r");
    if (file == NULL) {
        // If the file does not exist, return a 404 error
        const char *not_found_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "\r\n"
            "404 Not Found\r\n";
        send(client_socket, not_found_response, strlen(not_found_response), 0);
        return;
    }

    // Read the file content into a buffer
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_content = malloc(file_size);
    if (file_content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return;
    }
    fread(file_content, 1, file_size, file);
    fclose(file);

    // HTTP Response headers
    const char *http_header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "\r\n";  // Blank line between headers and body

    // Send the HTTP header with Content-Length
    char header_buffer[256];
    snprintf(header_buffer, sizeof(header_buffer), http_header, file_size);
    send(client_socket, header_buffer, strlen(header_buffer), 0);

    // Send the HTML file content
    send(client_socket, file_content, file_size, 0);

    // Free the allocated memory
    free(file_content);
}

int main(int argc, char **argv) {
    int parentfd; /* parent socket */
    int childfd; /* child socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char buf[BUFSIZE]; /* message buffer */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */

    /*
     * check command line arguments
     */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /*
     * socket: create the parent socket
     */
    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));

    /* this is an Internet address */
    serveraddr.sin_family = AF_INET;

    /* let the system figure out our IP address */
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* this is the port we will listen on */
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(parentfd, (struct sockaddr *) &serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    /*
     * listen: make this socket ready to accept connection requests
     */
    if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */
        error("ERROR on listen");

    /*
     * main loop: wait for a connection request, echo input line,
     * then close connection.
     */
    clientlen = sizeof(clientaddr);
    while (1) {
        /*
         * accept: wait for a connection request
         */
        childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
        if (childfd < 0)
            error("ERROR on accept");

        /*
         * gethostbyaddr: determine who sent the message
         */
        // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
        //  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        // if (hostp == NULL)
        //   error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");
        // printf("server established connection with (%s)\n", hostaddrp);

        /*
         * read: read input string from the client
         */
        bzero(buf, BUFSIZE);
        n = read(childfd, buf, BUFSIZE);
        if (n < 0)
            error("ERROR reading from socket");
        // printf("server received %d bytes\n", n);

        struct HTTPRequest request = http_request_constructor(buf);
        printf("request uri ==== %s\n", request.URI);
        if(request.Method == GET) {
            if(strcmp(request.URI, "/data") == 0) {
                send_json_response(childfd);
            }
            else {
                char *html_file = "front.html";
                send_html_response(childfd, html_file);
            }

            fflush(stdout);
        }
        else if(request.Method == POST) {
            if(strcmp(request.URI, "/submit") == 0) {
                send_connected_info(childfd, request.Body);
            }
        }
    }
}
