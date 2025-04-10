#include <bbt/http/HttpServer.hpp>

void EchoService(std::shared_ptr<bbt::http::detail::Context> c)
{
    auto msg = c->GetParam("msg");
    if (msg.empty()) {
        std::string msg_empty = "msg is empty!";

        auto err = c->SendReply(400, "Bad Request", bbt::core::Buffer("msg is empty!"));
        if (err != std::nullopt) {
            std::cerr << err.value().What() << std::endl;
        }
        return;
    }

    auto body = "{\"msg\":\"" + msg + "\"}";

    auto err = c->AddHeaderL("Content-Type", "application/json").
                SendReply(200, "OK", bbt::core::Buffer(body));

    if (err != std::nullopt) {
        std::cerr << err.value().What() << std::endl;
    }
}

int main()
{
    auto evthread = std::make_shared<bbt::pollevent::EvThread>();
    auto server = std::make_shared<bbt::http::HttpServer>();

    if (auto err = server->RunInEvThread(*evthread, "0.0.0.0", 11001); err != std::nullopt) {
        std::cerr << err.value().What() << std::endl;
        return -1;
    }

    if (auto err = server->Route("/echo", EchoService); err != std::nullopt) {
        std::cerr << err.value().What() << std::endl;
        return -1;
    }

    evthread->Start();
    std::cout << "Server is running..." << std::endl;
    sleep(5);
    if (auto err = server->UnRoute("/echo"); err != std::nullopt) {
        std::cerr << err.value().What() << std::endl;
        return -1;
    }

    std::cout << "unroute /echo" << std::endl;
    sleep(5);
    server->Route("/echo", EchoService);
    std::cout << "re-route /echo" << std::endl;

    sleep(5);

    server->Stop();
    
    evthread->Join();
}