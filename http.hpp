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