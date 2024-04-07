#pragma once

#include <concepts>
#include <expected>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/beast/ssl.hpp>

#include "detail/namespace_util.h"
#include "serialization.h"

namespace netlib {
    namespace http {
        using request = bhttp::request<bhttp::string_body>;
        using response = bhttp::response<bhttp::dynamic_body>;
        using bhttp::verb;
        using bhttp::field;

        ssl::context simple_client_ssl_context();

        template<typename Message, typename... Args>
        void parse(const Message& msg, Args&... args) {
            parse_value_from_string(msg.body(), args...);
        }

        class client {
        private:
            asio::io_context& m_context;
            ssl::context& m_ssl_context;
            beast::ssl_stream<beast::tcp_stream> m_stream;
            beast::flat_buffer m_buff;
        public:
            client(asio::io_context& context, ssl::context& ssl_context);
            void connect(std::string_view host_name);
            response send(const request& req);

            asio::awaitable<void> async_connect(std::string_view host_name);

            template<std::invocable<response> completion_handler>
            asio::awaitable<void> async_send(const request& req, completion_handler&& handler) {
                request resp;
                co_await bhttp::async_write(m_stream, req, asio::use_awaitable);
                co_await bhttp::async_read(m_stream, m_buff, resp, asio::use_awaitable);
                handler(std::move(resp));
            }
        };

        using request_handler = std::function<response(const request&)>;

        class server {
        private:
            asio::io_context& m_context;
            ssl::context& m_ssl_context;
            beast::ssl_stream<beast::tcp_stream> m_stream;
            std::unordered_map<std::string, request_handler> m_resp_handler;
            std::atomic_bool m_running = false;
        public:
            server(asio::io_context& context, ssl::context& ssl_context);

            template<std::convertible_to<std::string> string, std::invocable<const request&> resp_handler>
            void map(string&& resource, resp_handler&& handler) {
                m_resp_handler.emplace(std::forward<string>(resource), std::forward<resp_handler>(handler));
            }

            asio::awaitable<void> listen();
            void stop();
        };
    }
}