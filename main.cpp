#include "server.hpp"

#include <stdexcept>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& path) {

    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error(
            "Could not open file"
        );
    }

    std::stringstream buffer;

    buffer << file.rdbuf();

    return buffer.str();
}

int main() {

    Server server(8080);

    // GET /hello

    server.get(
        "/hello",
        [](const Request&, Response& res) {

            res.status_code = StatusCode::OK;

            res.body =
                "Hello from API!";

            res.headers["Content-Type"] =
                "text/plain";
        }
    );

    // GET /users

    server.get(
        "/users",
        [](const Request&, Response& res) {

            res.status_code = StatusCode::OK;

            res.body =
                "User list";

            res.headers["Content-Type"] =
                "text/plain";
        }
    );

    // POST /users

    server.post(
        "/users",
        [](const Request& req, Response& res) {

            res.status_code = StatusCode::OK;

            res.body =
                "Created user with body: "
                + req.body;

            res.headers["Content-Type"] =
                "text/plain";
        }
    );

    // PUT /users

    server.put(
        "/users",
        [](const Request& req, Response& res) {

            res.status_code = StatusCode::OK;

            res.body =
                "Updated user with body: "
                + req.body;

            res.headers["Content-Type"] =
                "text/plain";
        }
    );

    // DELETE /users

    server.del(
        "/users",
        [](const Request&, Response& res) {

            res.status_code = StatusCode::OK;

            res.body =
                "User deleted";

            res.headers["Content-Type"] =
                "text/plain";
        }
    );

    // GET /

    server.get(
        "/",
        [](const Request&, Response& res) {

            try {

                res.status_code =
                    StatusCode::OK;

                res.body =
                    readFile("./public/index.html");

                res.headers["Content-Type"] =
                    "text/html";
            }
            catch (const std::exception&) {

                res.status_code =
                    StatusCode::NotFound;

                res.body =
                    "File Not Found";

                res.headers["Content-Type"] =
                    "text/plain";
            }
        }
    );

    // GET /style.css

    server.get(
        "/style.css",
        [](const Request&, Response& res) {

            try {

                res.status_code =
                    StatusCode::OK;

                res.body =
                    readFile("./public/style.css");

                res.headers["Content-Type"] =
                    "text/css";
            }
            catch (const std::exception&) {

                res.status_code =
                    StatusCode::NotFound;

                res.body =
                    "File Not Found";

                res.headers["Content-Type"] =
                    "text/plain";
            }
        }
    );

    // 500 test

    server.get(
        "/error",
        [](const Request&, Response&) {

            throw std::runtime_error(
                "Intentional test error"
            );
        }
    );

    server.run();

    return 0;
}