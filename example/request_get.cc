#include <bbt/http/HttpClient.hpp>
#include <bbt/http/Request.hpp>


using namespace bbt;

int main()
{
    auto evthread = std::make_shared<bbt::pollevent::EvThread>(); 

    http::HttpClient client;

    if (auto err = client.RunInEvThread(*evthread); err) {
        std::cerr << "Error: " << err->What() << std::endl;
        return -1;
    }

    auto req = http::Request{};

    req.SetOpt(http::emHttpOpt::CURLOPT_URL, "http://www.example.com");
    req.SetOpt(http::emHttpOpt::CURLOPT_HTTPGET, 1L);
    req.SetOpt(http::emHttpOpt::CURLOPT_TIMEOUT_MS, 1000);

    req.SetResponseCallback([](core::errcode::ErrOpt err, http::Request* req) {
        if (err) {
            std::cerr << "Request failed: " << err->What() << std::endl;
        } else {
            std::cout << "Response Header: " << req->GetHeader().Peek() << std::endl;
            std::cout << "Response Body: " << req->GetBody().Peek() << std::endl;
        }
    });

    if (auto err = client.ProcessRequestEx(&req); err) {
        std::cerr << "Error: " << err->What() << std::endl;
        return -1;
    }

    evthread->Start();

    sleep(1200);

    evthread->Stop();
}