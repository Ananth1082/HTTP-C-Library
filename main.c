#include "lib/server.h"

void getMonsterController(struct HTTPRequest *req, struct HTTPResponseWriter *res)
{
	res->content_type = "application/json";
	res->status_code = 200;
	write_to_client(res, "{\"monsters\":[\"foo\",\"bar\",\"foobar\"]}\n");
}

int main() {
  struct HTTPRouter *router= newHTTPRouter(8080,2);
  register_route(router,GET,"/api/monsters",getMonsterController);
  printf("Routes: %d\n",router->Routes[0]->method);
  listen_requests(router);
  return 0;
}