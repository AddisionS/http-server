#pragma once

#include <string>
#include <unordered_map>

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

//response struct for reverting on requests 

struct Response{
    std::string version;
    int status_code;
    std::string reason;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

//method for sending response
std::string serializeResponse(const Response& response);

//mock response method
Response createResponse();