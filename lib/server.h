#ifndef SERVER_H
#define SERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#define MAX_BODY_SIZE 2048
#define MAX_ROUTES 10
#define EOH "\r\n\r\n"
#define ALL 0
#define GET 1
#define POST 2
#define PUT 3
#define DELETE 4

struct HTTPResponseWriter
{
	int status_code;
	int fd;
	char *content_type;
};

struct HTTPRequest
{
	char *path;
	int method;
	char *body;
};

typedef void (*controller_type)(struct HTTPRequest *, struct HTTPResponseWriter *);

struct HTTPRoute
{
	int method;
	char *path;
	controller_type controller;
};

struct HTTPRouter
{
	struct sockaddr_in server_addr;
	int server_fd;
	struct HTTPRoute **Routes;
	struct sockaddr_in client_addr;
	int route_count;
};

struct HTTPRouter *newHTTPRouter(const int port);
void listen_requests(struct HTTPRouter *router);
void register_route(struct HTTPRouter *router, const int method, const char *path, controller_type ctrl);
void write_to_client(struct HTTPResponseWriter *res, char *reply);

#endif