#include <bbt/http/detail/ReqParser.hpp>
#include <bbt/http/detail/RespParser.hpp>

const char request[] = "GET / HTTP/1.1\r\nHost: www.example.com\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n";
const char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nCustom-Field: any\r\n\r\n<html>...</html>";

int main()
{

    bbt::http::detail::ReqParser req_parser;
    bbt::http::detail::RespParser resp_parser;

    // Parse request
    auto req_err = req_parser.ExecuteParse(request, sizeof(request) - 1);
    if (req_err) {
        std::cerr << "Request parse error: " << req_err->What() << std::endl;
        return 1;
    }
    
    std::cout << "Request URL: " << req_parser.GetUrl() << std::endl;
    std::cout << "Request Method: " << req_parser.GetMethod() << std::endl;
    std::cout << "Request Version: " << req_parser.GetVersion() << std::endl;
    std::cout << "Request Headers: " << std::endl;
    for (const auto& header : req_parser.GetHeaders()) {
        std::cout << "  " << header.first << ": " << header.second << std::endl;
    }

    // Parse response
    auto resp_err = resp_parser.ExecuteParse(response, sizeof(response) - 1);
    if (resp_err) {
        std::cerr << "Response parse error: " << resp_err->CWhat() << std::endl;
        return 1;
    }

    std::cout << "Response Version: " << resp_parser.GetVersion() << std::endl;
    std::cout << "Response Status: " << resp_parser.GetStatus() << std::endl;
    std::cout << "Response Headers: " << std::endl;
    for (const auto& header : resp_parser.GetHeaders()) {
        std::cout << "  " << header.first << ": " << header.second << std::endl;
    }


    return 0;

}