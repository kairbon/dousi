
#include "core/executor/connection_session.h"
#include "common/logging.h"

#include <msgpack.hpp>

namespace dousi {
namespace executor {

void ConnectionSession::DoReadObjectID() {
    // This self is used for the case that this self could be destroyed before the lambda performed.
    auto self(shared_from_this());
    std::shared_ptr<uint32_t> object_id_ptr = std::make_shared<uint32_t>();
    constexpr size_t HEADER_SIZE = sizeof(*object_id_ptr);
    // Read head first.
    // We should make sure the endian here. Otherwise the value will be incorrect.
    boost::asio::async_read(
            socket_,
            boost::asio::buffer(reinterpret_cast<char *>(object_id_ptr.get()), HEADER_SIZE),
            [this, object_id_ptr, self](boost::system::error_code error_code, size_t length) {
                // ASSERT(length == HEADER_SIZE);
                if (error_code) {
                    DOUSI_LOG(INFO) << "Failed to receive object_id with error code: " << error_code.message();
                    return;
                }
                DOUSI_LOG(INFO) << "Succeeded to receive the object_id : " << *object_id_ptr;
                DoReadHeader(*object_id_ptr);
            });
}

void ConnectionSession::DoReadHeader(uint32_t object_id) {
    // This self is used for the case that this self could be destroyed before the lambda performed.
    auto self(shared_from_this());
    std::shared_ptr<uint32_t> body_size_ptr = std::make_shared<uint32_t>();
    constexpr size_t HEADER_SIZE = sizeof(*body_size_ptr);
    // Read head first.
    // We should make sure the endian here. Otherwise the value will be incorrect.
    boost::asio::async_read(
            socket_,
            boost::asio::buffer(reinterpret_cast<char *>(body_size_ptr.get()), HEADER_SIZE),
            [this, object_id, body_size_ptr, self](boost::system::error_code error_code, size_t length) {
                // ASSERT(length == HEADER_SIZE);
                if (error_code) {
                    DOUSI_LOG(INFO) << "Failed to receive header with error code: " << error_code;
                    return;
                }
                DOUSI_LOG(INFO) << "Succeeded to receive the header: " << *body_size_ptr;
                DoReadBody(object_id, *body_size_ptr);
            });
}

void ConnectionSession::DoReadBody(uint32_t object_id, uint32_t body_size) {
    auto self {shared_from_this()};
    std::shared_ptr<char> buffer_ptr(new char[body_size], std::default_delete<char[]>());

    boost::asio::async_read(
            socket_,
            boost::asio::buffer(buffer_ptr.get(), body_size),
            [buffer_ptr, this, object_id, self](boost::system::error_code error_code, size_t length) {
                if (error_code) {
                    DOUSI_LOG(INFO) << "Failed to receive body with error code: " << error_code.message();
                    return;
                }

                std::string data(buffer_ptr.get(), length);
                DOUSI_LOG(INFO) << "Succeeded to receive the body: " << data;
                std::string result;
                invocation_callback_(conn_id_, data, result);

                {
                    // Write object id first.
                    char header[sizeof(object_id)];
                    memcpy(header, &object_id, sizeof(object_id));
                    boost::asio::async_write(
                            socket_,
                            boost::asio::buffer(header, sizeof(object_id)),
                            [this, object_id](boost::system::error_code error_code, size_t) {
                                if (error_code) {
                                    socket_.close();
                                    DOUSI_LOG(INFO) << "Failed to write object_id to server with error code:" << error_code.message();
                                } else {
                                    DOUSI_LOG(DEBUG) << "Succeeded to write object_id to server, object_id=" << object_id;
                                }
                            });
                }

                uint32_t result_size = result.size();
                {
                    // Then write header.
                    char header[sizeof(result_size)];
                    memcpy(header, &result_size, sizeof(result_size));
                    boost::asio::async_write(
                            socket_,
                            boost::asio::buffer(header, sizeof(result_size)),
                            [this, result_size](boost::system::error_code error_code, size_t) {
                                if (error_code) {
                                    socket_.close();
                                    DOUSI_LOG(INFO) << "Failed to write header to server with error code:" << error_code;
                                } else {
                                    DOUSI_LOG(DEBUG) << "Succeeded to write header to server, header=" << result_size;
                                }
                            });
                }
                {
                    // Write body.
                    socket_.async_send(
                            boost::asio::buffer(result.data(), result.size()),
                            [result_size](const boost::system::error_code &error, std::size_t bytes_transferred) {
                                if (result_size != bytes_transferred) {
                                    DOUSI_LOG(INFO) << "erroring, bytes_transferred=" << bytes_transferred
                                                    << ", but result_size=" << result_size;
                                    return;
                                }
                                DOUSI_LOG(INFO) << "Succeeded to send response to client.";
                            });
                }
                DoReadObjectID();
            });
}

void ConnectionSession::Write(const std::string &data) {
    // TODO(qwang): We should write the size of the data first.
    socket_.async_send(
            boost::asio::buffer(data.data(), data.size()),
            [size = data.size()](const boost::system::error_code &error, size_t sent_bytes) {
                assert(size == sent_bytes);
                if (error) {
                    DOUSI_LOG(INFO) << "Failed to write message to the connection with error " << error.message();
                    return;
                }
            });
}

}

}