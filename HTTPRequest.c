#include"HTTPRequest.h"
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int method_select(char *method)
{

    if(strcmp(method, "GET") == 0) {
        return GET;
    }
    else if(strcmp(method, "POST") == 0) {
        return POST;
    }
    else if(strcmp(method, "PUT") == 0) {
        return PUT;
    }
    return TRACE;
}

struct HTTPRequest http_request_constructor(char *request_string) {
    struct HTTPRequest request;
    for(int i=0; i < strlen(request_string)-1; i++) {
        if(request_string[i]=='\n' && request_string[i+1] == '\n') {
            request_string[i+1] = '|';
        }
    }
    char *request_line = strtok(request_string, "\n");
    char *body;

    // header fields and body are not extracted properly
    char *token = strtok(NULL, "\r\n\r\n");
    while(token != NULL) {

        // request body is little ambiguous, sometime header will come as body
        // TODO: Fix the body part ...
        body = token;
        token = strtok(NULL, "\r\n\r\n");
    }


    char *method = strtok(request_line, " ");
    request.Method = method_select(method);

    char *URI = strtok(NULL, " ");
    request.URI = URI;

    char *HTTPVersion = strtok(NULL, " ");
    HTTPVersion = strtok(HTTPVersion, "/");
    HTTPVersion = strtok(NULL, "/");
    request.HTTPVersion = (float)atof(HTTPVersion);
    request.Body = body;

    return request;
}
