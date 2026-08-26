#include "http.hpp"
#include <stdexcept>

//parse request method implementation 
Request parseRequest(const std::string& raw) {
    Request request;

    //request line parsing 
    size_t first_space = raw.find(" ");

    if (first_space == std::string::npos) {
        throw std::runtime_error("Malformed request line");
    }

    size_t second_space = raw.find(' ', first_space + 1);

    if (second_space == std::string::npos) {
        throw std::runtime_error("Malformed request line");
    }

    request.method = raw.substr(0, first_space);

    request.path = raw.substr(
        first_space + 1,
        second_space - first_space - 1
    );

    request.version = raw.substr(second_space + 1);

    //find header end 
    size_t headers_end = raw.find("\r\n\r\n");

    if (headers_end == std::string::npos) {
        return request;
    }

    size_t line_start = raw.find("\r\n") + 2;

    //parseing header
    while(line_start < headers_end) {
        size_t line_end = raw.find("\r\n", line_start);

        if (line_end == std::string::npos) {
            break;
        }

        std::string line = raw.substr(
            line_start,
            line_end - line_start
        );

        size_t colon = line.find(':');

        if (colon != std::string::npos) {

            std::string key = line.substr(0, colon);

            std::string value = line.substr(colon + 1);

            // Remove leading space from value
            if (!value.empty() && value[0] == ' ') {
                value.erase(0, 1);
            }

            request.headers[key] = value;
        }

        line_start = line_end + 2;
    }

    return request; 
}

//method to return status code in int
int statusCodeToInt(StatusCode status) {
    return static_cast<int>(status);
}

//status code to reason
std::string statusText(StatusCode status) {
    switch (status) {
        case StatusCode::OK:
            return "OK";

        case StatusCode::BadRequest:
            return "Bad Request";

        case StatusCode::NotFound:
            return "Not Found";

        case StatusCode::MethodNotAllowed:
            return "Method Not Allowed";

        case StatusCode::InternalServerError:
            return "Internal Server Error";
    }

    return "Unknown";
}

//response sending method implementation
std::string serializeResponse(const Response& response){
    std::string response_line;

    response_line += response.version;
    response_line += " ";
    response_line += std::to_string(statusCodeToInt(response.status_code));
    response_line += " ";
    response_line += statusText(response.status_code);
    response_line += "\r\n";

    for (const auto& [key, value] : response.headers) {
        response_line += key;
        response_line += ": ";
        response_line += value;
        response_line += "\r\n";
    }
    response_line += "\r\n";

    response_line += response.body;

    return response_line;
}


//method to add routes
void Router::addRoute(const std::string& method, const std::string& path, Handler handler) {
    routes[{method, path}] = handler;
}

//method to find route
Handler* Router::findRoute(const std::string& method, const std::string& path) {
    auto it = routes.find({method, path});

    if (it != routes.end()) {
        return &it->second;
    }

    return nullptr;
}

//method to check path's existence 
bool Router::pathExists(const std::string& path) {
    for (const auto& route : routes) {
        if (route.first.second == path) {
            return true;
        }
    }

    return false;
}
