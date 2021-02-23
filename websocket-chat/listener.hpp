#pragma once

#include <websocket-chat/common.hpp>
#include <memory>

class SharedState;

class Listener : public std::enable_shared_from_this<Listener> {
public:
	Listener(net::io_context& ioContext, net::ip::tcp::endpoint endpoint, const std::shared_ptr<SharedState>& state);
	void run();

private:
	void onAccept(beast::error_code errorCode, net::ip::tcp::socket socket);

	static void fail(beast::error_code errorCode, const char* what);
private:
	net::io_context& _ioContext;
	net::ip::tcp::acceptor _acceptor;
	std::shared_ptr<SharedState> _state;
};
