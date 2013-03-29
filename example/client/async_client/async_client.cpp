﻿#include <iostream>
#include <boostconnect/config.hpp>
#include <boostconnect/connection_type/async_connection.hpp>
#include <boostconnect/client.hpp>

int main()
{
    typedef boost::system::error_code error_code;
    typedef bstcon::client::response_type response_type; // = boost::shared_ptr<bstcon::response>
    typedef bstcon::client::connection_ptr connection_ptr;

    // Make: request header and body
    boost::shared_ptr<boost::asio::streambuf> request_keep(new boost::asio::streambuf());
    {
        std::ostream os(request_keep.get());
        os << "GET / HTTP/1.1\r\n";
        os << "Host: www.google.co.jp\r\n";
        os << "Connection: Keep-Alive\r\n";
        os << "\r\n";
    }
    boost::shared_ptr<boost::asio::streambuf> request_close(new boost::asio::streambuf());
    {
        std::ostream os(request_close.get());
        os << "GET / HTTP/1.1\r\n";
        os << "Host: www.google.co.jp\r\n";
        os << "Connection: close\r\n";
        os << "\r\n";
    }

    // Make: client using async 
    boost::asio::io_service io_service;
    bstcon::client client(
        io_service,
        bstcon::connection_type::async // important!
        );

    // Challenge TCP connection establishment
    bstcon::client::connection_ptr connection = client(
        std::string("www.google.co.jp"),
        [&](bstcon::client::connection_ptr connection_inner, boost::system::error_code ec)->void // Callback
        {
            assert(connection == connection_inner); // guarantee that

            if(ec)
            {
                // Bad connection
                throw;
            }

            // Nested lambda
            auto connection_ = connection;
            auto request_close_ = request_close;

            // Send First request
            connection->send(
                request_keep,
                [connection_, request_close_](response_type response, boost::system::error_code ec)->void // Callback
                {
                    if(ec && ec != boost::asio::error::eof)
                    {
                        // can't send request or read response
                        throw;
                    }

                    // Show first response
                    std::cout << "Status Code: " << response->status_code << " " << response->status_message << std::endl;
                    std::cout << response->body + "\n\n" <<std::endl;

                    // Send Second request (Continue request)
                    connection_->send(
                        request_close_,
                        [](bstcon::client::response_type response, boost::system::error_code ec)->void  // Callback
                        {
                            if(ec && ec != boost::asio::error::eof) throw;

                            // Show second response
                            std::cout << "Status Code: " << response->status_code << " " << response->status_message << std::endl;
                            std::cout << response->body + "\n\n" << std::endl;
                        }
                    );
                }
            );
        }
    );

    // Run the above
    io_service.run();

    return 0;
}
