#include <bbt/http/HttpServer.hpp>

int main()
{
    auto evthread = std::make_shared<bbt::pollevent::EvThread>();
    auto server = std::make_shared<bbt::http::HttpServer>();

    if (auto err = server->RunInEvThread(*evthread, "0.0.0.0", 8080); err != std::nullopt) {
        std::cerr << err.value().What() << std::endl;
        return -1;
    }

    if (auto err = server->Route("/test", [](std::shared_ptr<bbt::http::detail::Context> c) {
        auto request = c->GetRequestFieldData();

        std::cout << "Request URL: " << request.m_url << std::endl;
        std::cout << "Request Status: " << request.m_status << std::endl;
        std::cout << "Request Body: " << request.m_body << std::endl;
        std::cout << "Request Header: " << std::endl;
        for (const auto& [key, value] : request.m_kv_http_response) {
            std::cout << key << ": " << value << std::endl;
        }

        auto body = R"({"key": "value"})";

        auto err = c->AddHeaderL("Content-Type", "application/json").
                        AddHeaderL("Content-Length", std::to_string(strlen(body)).c_str()).
                    SendReply(200, "OK", bbt::core::Buffer(body));

        if (err != std::nullopt) {
            std::cerr << err.value().What() << std::endl;
        }
    }); err != std::nullopt) {
        std::cerr << err.value().What() << std::endl;
        return -1;
    }

    evthread->Start();

    std::cout << "Server is running..." << std::endl;

    sleep(100);
}