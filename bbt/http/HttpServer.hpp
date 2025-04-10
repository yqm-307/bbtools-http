#pragma once
#include <unordered_map>
#include <atomic>
#include <bbt/http/detail/Define.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/http/detail/Context.hpp>



namespace bbt::http
{

class HttpServer:
    public std::enable_shared_from_this<HttpServer>
{
public:
    typedef std::function<void(std::shared_ptr<detail::Context> resp)> ReqHandler;

    HttpServer();
    ~HttpServer();

    /**
     * @brief 将Server运行在指定端口、地址、evthread中，请不要在多线程中使用
     * 
     * @param evthread 
     * @param ip 
     * @param port 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt   RunInEvThread(pollevent::EvThread& evthread, const std::string& ip, short port);

    /**
     * @brief 注册一个http请求的处理函数（线程安全）
     * 
     * @param uri 
     * @param handle 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt   Route(const std::string& uri, const ReqHandler& handle);

    /**
     * @brief 反注册一个http请求处理函数（线程安全）
     * 
     * @param uri 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt   UnRoute(const std::string& uri);

    /**
     * @brief 停止服务器
     * 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt   Stop();

    /**
     * @brief 异步发送响应（线程安全）
     * 
     * @param resp 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt   AsyncReply(std::shared_ptr<detail::Context> c);

    /**
     * @brief 设置一个错误处理函数
     * 
     * @param on_err 
     */
    void                    SetErrCallback(const OnErrorCallback& on_err);
protected:
    core::errcode::ErrOpt   _BindAddress(const std::string& ip, short port);
    void                    _ProcessSendReply();

    static void             OnRequest(evhttp_request* req, void* arg);
private:
    struct OnReqHandle {
        std::weak_ptr<HttpServer> server;
        ReqHandler handle;
    };

    std::mutex              m_all_opt_mtx;
    evhttp*                 m_http_server{NULL};
    volatile bool           m_is_running{false};
    OnErrorCallback         m_onerr{nullptr};
    std::shared_ptr<pollevent::Event> m_send_reply_event{nullptr};

    std::unordered_map<std::string, OnReqHandle*> m_handles;
    std::queue<std::shared_ptr<detail::Context>> m_async_send_response_queue;    // 异步发送响应队列
    
};
} // namespace bbt::http