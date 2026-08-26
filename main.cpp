#include "server.hpp"

int main() {

    Router router;

    router.addRoute(
        "GET",
        "/hello",
        [](const Request&, Response& res) {
            res.status_code = StatusCode::OK;
            res.body = "Hello from router!";
            res.headers["Content-Type"] = "text/plain";
        }
    );

    Server server(8080, router);

    server.run();

    return 0;
}