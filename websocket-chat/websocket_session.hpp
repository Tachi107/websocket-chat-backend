#pragma once

#include <websocket-chat/common.hpp>
#include <websocket-chat/shared_state.hpp>
#include <iostream>

class SharedState;

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession> {
public:
	WebsocketSession(net::ip::tcp::socket&& socket, const std::shared_ptr<SharedState>& state);

	~WebsocketSession();

// Funzione che prende la richiesta HTTP di upgrade e accetta lo scambio dati ws
template<typename Body, typename Allocator>
void run(beast::http::request<Body, beast::http::basic_fields<Allocator>> request);

void send(const std::shared_ptr<const std::string>& sharedString);

private:
void onAccept(beast::error_code errorCode);

void onRead(beast::error_code errorCode, std::size_t bytes);

void onWrite(beast::error_code errorCode, std::size_t bytes);

void onSend(const std::shared_ptr<const std::string>& sharedString);

static void fail(beast::error_code errorCode, const char* what);

private:
	beast::flat_buffer _buffer;
	beast::websocket::stream<net::ip::tcp::socket> _websocketStream; // Contiene stato ws
	std::shared_ptr<SharedState> _state;
	std::vector<std::shared_ptr<const std::string>> _messageQueue;
};

template<typename Body, typename Allocator>
void WebsocketSession::run(beast::http::request<Body, beast::http::basic_fields<Allocator>> request) {
	// Imposto il timeout consigliato per il Websocket
	_websocketStream.set_option(
		beast::websocket::stream_base::timeout::suggested(beast::role_type::server)
	);

	_websocketStream.async_accept(
		request,
		beast::bind_front_handler(
			&WebsocketSession::onAccept,
			shared_from_this()
		)
	);
}
