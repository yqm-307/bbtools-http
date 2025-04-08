#pragma once
#include <atomic>
#include <bbt/http/detail/Define.hpp>
#include <bbt/core/buffer/Buffer.hpp>

namespace bbt::http
{

class Request
{
public:
    Request();
    ~Request();

    void Clear();

    bool IsCompleted();

    core::errcode::ErrOpt SetOpt(emHttpOpt opt, ...);
    void SetResponseCallback(const ResponseCallback& cb);

    CURL* GetCURL();

    const bbt::core::Buffer& GetHeader() const;
    const bbt::core::Buffer& GetBody() const;

private: // friend
    friend size_t OnRecvHeader(void* buf, size_t size, size_t nmemb, void* arg);
    friend size_t OnRecvBody(void* buf, size_t size, size_t nmemb, void* arg);
    friend class HttpClient;

    void OnRecvHeader(const char* header, size_t len);
    void OnRecvBody(const char* body, size_t len); 
    void OnComplete(core::errcode::ErrOpt err);

private:
    // curl_easy_setopt


private:
    CURL* m_curl{NULL};
    // 目前简单处理，copy接收所有数据
    bbt::core::Buffer m_response_header{8};
    bbt::core::Buffer m_response_body{8};

    ResponseCallback  m_on_resp_cb{nullptr};

    std::atomic_bool  m_is_completed{false};
};

} // namespace bbt::http