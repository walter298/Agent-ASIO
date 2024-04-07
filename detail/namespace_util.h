#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/pfr.hpp>

namespace netlib {
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace bhttp = beast::http;
    namespace ssl = asio::ssl;
    namespace ip = asio::ip;
    using ip::tcp;
    namespace pfr = boost::pfr;
    namespace sys = boost::system;
}