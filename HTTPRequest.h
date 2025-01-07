// HTTPRequest.h
// Created by Ayush Garg

#ifndef HTTPRequest_h
#define HTTPRequest_h

enum HTTPMethods {
  GET,
  POST,
  PUT,
  HEAD,
  PATCH,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE
};

struct HTTPRequest {
  int Method;
  char *URI;
  float HTTPVersion;
  char *Body;
};

struct HTTPRequest http_request_constructor(char *request_string);

#endif
