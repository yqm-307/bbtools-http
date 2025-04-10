#include <bbt/http/HttpClient.hpp>
#include <bbt/http/Request.hpp>
#include <bbt/pollevent/EvThread.hpp>

int main()
{
    bbt::http::HttpClient client;
    auto thread = std::make_shared<bbt::pollevent::EvThread>();

    if (auto err = client.RunInEvThread(*thread); err.has_value()) {
        std::cerr << err.value().What() << std::endl;
        return -1;
    }


    auto event = thread->RegisterEvent(-1, EV_PERSIST | EV_TIMEOUT, [&client](int fd, short event, bbt::pollevent::EventId id) {
        auto req = std::make_shared<bbt::http::Request>();

        req->SetOpt(bbt::http::emHttpOpt::CURLOPT_URL, "http://127.0.0.1:11001/testc");
        req->SetOpt(bbt::http::emHttpOpt::CURLOPT_HTTPPOST, 1L);
        req->SetOpt(bbt::http::emHttpOpt::CURLOPT_POSTFIELDS, R"({"key":"value"})");
        req->SetResponseCallback([req](auto err){
            if (err.has_value()) {
                std::cerr << "Request failed: " << err.value().What() << std::endl;
                return;
            }
            else {
                std::cout << req->GetRawResponse().Peek() << std::endl;
            }
        });

        if (auto err = client.ProcessRequestEx(req); err.has_value()) {
            std::cerr << "Error: " << err.value().What() << std::endl;
            return;
        }
    });

    event->StartListen(200);

    
    thread->Start();
    thread->Join();
}