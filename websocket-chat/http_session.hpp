#pragma once

#include <websocket-chat/common.hpp>
#include <websocket-chat/shared_state.hpp>

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
	HttpSession(net::ip::tcp::socket&& socket, const std::shared_ptr<SharedState>& state);
	void run();

private:
	void onRead(beast::error_code errorCode, std::size_t /*bytes*/);
	void onWrite(beast::error_code errorCode, std::size_t /*bytes*/, bool close);
	void doRead();
	static void fail(beast::error_code errorCode, const char* what);

private:
	beast::tcp_stream _tcpStream;
	beast::flat_buffer _buffer;
	std::shared_ptr<SharedState> _state;
	// Memorizzato come optional così che possa costruirne uno per ogni richiesta (ha senso?)
	std::optional<beast::http::request_parser<beast::http::string_body>> _parser;
};
