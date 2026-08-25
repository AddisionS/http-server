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

//response sending method implementation
std::string serializeResponse(const Response& response){
    std::string response_line;

    response_line += response.version;
    response_line += " ";
    response_line += std::to_string(response.status_code);
    response_line += " ";
    response_line += response.reason;
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


//mock response maker method implementation
Response createResponse() {
    Response response;

    response.version = "HTTP/1.1";
    response.status_code = 200;
    response.reason = "OK";
    response.body = "Hello";

    response.headers["Content-Type"] = "text/plain";
    response.headers["Content-Length"] =
        std::to_string(response.body.size());

    return response;
}