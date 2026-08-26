#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <map>
#include <utility>



//request struct for parsing request

struct Request{
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

//method for parsing request 
Request parseRequest(const std::string& raw);

//enum for status codes 
enum class StatusCode {
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    MethodNotAllowed = 405,
    InternalServerError = 500
};

//response struct for reverting on requests 

struct Response{
    std::string version;
    StatusCode status_code;
    std::string reason;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

//method for sending response
std::string serializeResponse(const Response& response);

//mock response method
Response createResponse();

using Handler = std::function<void(const Request&, Response&)>;
//routing class
class Router {
    private:
        std::map<std::pair<std::string, std::string>, Handler> routes;

    public:
        void addRoute(
            const std::string& method,
            const std::string& path,
            Handler handler
        );

        Handler* findRoute(
            const std::string& method,
            const std::string& path
        );

        bool pathExists(const std::string& path);
};