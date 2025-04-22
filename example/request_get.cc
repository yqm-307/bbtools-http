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

    auto req = std::make_shared<http::Request>();

    req->SetOpt(http::emHttpOpt::CURLOPT_URL, "http://www.example.com");
    req->SetOpt(http::emHttpOpt::CURLOPT_HTTPGET, 1L);
    req->SetOpt(http::emHttpOpt::CURLOPT_TIMEOUT_MS, 1000);

    req->SetResponseCallback([req](auto err) {
        if (err) {
            std::cerr << "Request failed: " << err->What() << std::endl;
            return;
        }

        if (req->IsCompleted()) {
            auto [err, parser] = req->Parse();
            if (err) {
                std::cerr << "Parse error: " << err->What() << std::endl;
                return;
            }
            else {
                std::cout << "Request Version: " << parser->GetVersion() << std::endl;
                std::cout << "Response Status: " << parser->GetStatus() << std::endl;
                std::cout << "Response Body: " << parser->GetBody() << std::endl;
                std::cout << "Response Header: " << std::endl;
                for (const auto& [key, value] : parser->GetHeaders()) {
                    std::cout << key << ": " << value << std::endl;
                }
            }
        }
    });

    // 执行一次 Get 请求
    if (auto err = client.ProcessRequestEx(req); err) {
        std::cerr << "Error: " << err->What() << std::endl;
        return -1;
    }

    evthread->Start();

    sleep(1200);

    evthread->Stop();
}