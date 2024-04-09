#include "http.h"

namespace netlib {
	namespace http {
		ssl::context simple_client_ssl_context() {
			ssl::context ret{ ssl::context::tlsv12_client };
			ret.set_default_verify_paths();
			return ret;
		}

		client::client(asio::io_context& context, ssl::context& ssl_context) 
			: m_context{ context }, m_ssl_context{ ssl_context }, 
			m_stream { context, ssl_context }
		{
		}
		void client::connect(std::string_view host_name) {
			if (!SSL_set_tlsext_host_name(m_stream.native_handle(), host_name.data())) {
				beast::error_code ec{ static_cast<int>(::ERR_get_error()), asio::error::ssl_category };
				throw beast::system_error{ ec };
			}
			tcp::resolver res{ m_context };
			auto results = res.resolve(host_name, "https");
			beast::get_lowest_layer(m_stream).connect(results);
			m_stream.handshake(ssl::stream_base::client);
		}
		response client::send(const request& req) {
			response resp;
			bhttp::write(m_stream, req);
			bhttp::read(m_stream, m_buff, resp);
			return resp;
		}
		asio::awaitable<void> client::async_connect(std::string_view host_name) {
			tcp::resolver res{ m_context };
			auto results = co_await res.async_resolve(host_name, "https", asio::use_awaitable);
			co_await m_stream.async_handshake(ssl::stream_base::client, asio::use_awaitable);
		}

		server::server(asio::io_context& context, ssl::context& ssl_context)
			: m_context{ context }, m_ssl_context{ ssl_context },
			m_stream{ context, ssl_context }
		{
		}

		asio::awaitable<void> server::listen() {
			beast::flat_buffer buff;
			m_running.store(true);
			while (m_running.load()) {
				request req;
				co_await bhttp::async_read(m_stream, buff, req, asio::use_awaitable);

				auto handler_KV = m_resp_handler.find(req.target());
				if (handler_KV == m_resp_handler.end()) {
					std::cerr << "Unhandled Response: " << req.target() << '\n';
				} else {
					const auto& handler = handler_KV->second;
					auto resp = handler(req);
					bhttp::write(m_stream, req);
				}
			}
			co_return;
		}

		void server::stop() {
			m_running.store(false);
		}
	}
}