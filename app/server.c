#include "server.h"


struct HTTPRouter *newHTTPRouter(const int port)
{
	struct HTTPRouter *router = malloc(sizeof(struct HTTPRouter));
	if (!router)
	{
		printf("Error allocating memory to router\n");
		exit(1);
	}
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	// Setting SO_REUSEADDR
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = {
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr = {htonl(INADDR_ANY)},
	};

	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0)
	{
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	router->server_addr = serv_addr;
	router->server_fd = server_fd;
	router->Routes = malloc(MAX_ROUTES * sizeof(struct HTTPRoute *));
	router->route_count = 0;
	return router;
}

void listen_requests(struct HTTPRouter *router)
{
	while (1)
	{
		printf("Server listening for requests\n");
		int client_addr_len = sizeof(struct sockaddr);

		int fd = accept(router->server_fd, &router->client_addr, &client_addr_len);
		printf("Client connected\n");

		char *req = malloc(2024);
		int req_size = recv(fd, req, 2024, 0);
		if (req_size == 0)
		{
			printf("Empty request");
			free(req);
			continue;
		}
		req[req_size] = '\0';

		struct HTTPRequest *req_parsed = malloc(sizeof(struct HTTPRequest));
		parse_request(req_parsed, req);
		free(req);
		if (!req_parsed)
		{
			printf("Invalid request\n");
			close(fd);
			continue;
		}

		printf("\nRequest details:\n\tPATH: %s\n\tMETHOD: %d\n\tBODY: %s\n", req_parsed->path, req_parsed->method, req_parsed->body);

		if (!execute_routes(router, req_parsed, fd))
		{
			printf("Not found 404 error\n");
		}
		free(req_parsed->path);
		free(req_parsed->body);
		free(req_parsed);
		close(fd);
	}
	close(router->server_fd);
}

void register_route(struct HTTPRouter *router, const int method, const char *path, controller_type ctrl)
{
	if (router->route_count >= MAX_ROUTES)
	{
		printf("Max routes reached for the router\n");
		return;
	}
	struct HTTPRoute *new_route = malloc(sizeof(struct HTTPRoute));
	new_route->method = method;
	new_route->path = strdup(path);
	new_route->controller = ctrl;
	router->Routes[router->route_count++] = new_route;
}

int execute_routes(struct HTTPRouter *router, struct HTTPRequest *req, int fd)
{
	for (int i = 0; i < router->route_count; i++)
	{
		if (router->Routes[i]->method == req->method && strcmp(router->Routes[i]->path, req->path) == 0)
		{
			struct HTTPResponseWriter *res = malloc(sizeof(struct HTTPResponseWriter));
			res->fd = fd;
			router->Routes[i]->controller(req, res);
			return 1;
		}
	}
	return 0;
}

void parse_request(struct HTTPRequest *request, char *req)
{
	// Make a copy of the original request since strtok modifies the string
	char *req_copy = strdup(req);
	char *req_line = req_copy;

	// Find the end of the first line
	char *end_of_line = strstr(req_copy, "\r\n");
	if (!end_of_line)
	{
		fprintf(stderr, "Invalid request format\n");
		free(req_copy);
		return;
	}
	*end_of_line = '\0';

	char *token = strtok(req_line, " ");
	if (!token)
	{
		fprintf(stderr, "Invalid request line format\n");
		free(req_copy);
		return;
	}
	char *method = strdup(token);
	if (!strcmp(method, "GET"))
	{
		request->method = GET;
	}
	if (!strcmp(method, "POST"))
	{
		request->method = POST;
	}
	if (!strcmp(method, "PUT"))
	{
		request->method = PUT;
	}
	if (!strcmp(method, "DELETE"))
	{
		request->method = DELETE;
	}
	// Parse path
	token = strtok(NULL, " ");
	if (!token)
	{
		fprintf(stderr, "Invalid request line format\n");
		free(req_copy);
		return;
	}
	request->path = strdup(token);

	// Find the body in the original request
	char *body_start = strstr(req, EOH);
	if (body_start)
	{
		body_start += 4; // Skip past \r\n\r\n
		request->body = strdup(body_start);
	}
	else
	{
		request->body = strdup(""); // Empty body
	}

	free(req_copy);
}

void write_to_client(struct HTTPResponseWriter *res, char *reply)
{
	int content_length = strlen(reply);
	char status_str[4];
	sprintf(status_str, "%d", res->status_code);

	char *response = malloc(2048);

	strcpy(response, "HTTP/1.1 ");
	strcat(response, status_str);
	strcat(response, " ");

	switch (res->status_code)
	{
	case 200:
		strcat(response, "Okie");
		break;
	case 404:
		strcat(response, "Not Found");
		break;
	default:
		strcat(response, "Unknown");
	}

	strcat(response, "\r\nContent-Type: ");
	strcat(response, res->content_type);
	strcat(response, "\r\nContent-Length: ");

	char length_str[16];
	sprintf(length_str, "%d", content_length);
	strcat(response, length_str);

	strcat(response, "\r\n\r\n");

	strcat(response, reply);

	int bytes_sent = send(res->fd, response, strlen(response), 0);
	if (bytes_sent == -1)
	{
		printf("Error sending response: %s\n", strerror(errno));
	}

	free(response);
}