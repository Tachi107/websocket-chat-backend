#include <websocket-chat/http_session.hpp>
#include <websocket-chat/websocket_session.hpp>
#include <iostream>

HttpSession::HttpSession(net::ip::tcp::socket&& socket, const std::shared_ptr<SharedState>& state)
	: _tcpStream(std::move(socket)), _state(state) {}

void HttpSession::run() {
	doRead();
}

void HttpSession::doRead() {
	using namespace std::chrono_literals;
	_parser.emplace();
	_tcpStream.expires_after(30s);

	beast::http::async_read(
		_tcpStream,
		_buffer,
		_parser->get(),
		beast::bind_front_handler(&HttpSession::onRead, shared_from_this())
	);
}

void HttpSession::onRead(beast::error_code errorCode, std::size_t /*bytes*/) {
	// Il client ha chiuso la connessione
	if (errorCode == beast::http::error::end_of_stream) {
		_tcpStream.socket().shutdown(net::ip::tcp::socket::shutdown_send, errorCode);
		return;
	}
	else if (errorCode) {
		return fail(errorCode, "onRead");
	}
	// I websocket vengono inizializzati con un'apposita richiesta HTTP
	else if (beast::websocket::is_upgrade(_parser->get())) {
		// Creo quindi una sessione Websocket spostando il socket
		// dalla sessione HTTP a quella WS, insieme alla richiesta,
		// così che la WebsocketSession ne diventi padrona
		std::make_shared<WebsocketSession>(
			_tcpStream.release_socket(),
			_state
		)->run(_parser->release());
		// Ritorno, così che la sessione HTTP venga automaticamente distrutta
		return;
	}
	//else {
		// Gestisco la richiesta HTTP, servendo ad esempio un file
	//	handleRequest();
	//}
}

void HttpSession::onWrite(beast::error_code errorCode, std::size_t /*bytes*/, bool close) {
	if (errorCode) {
		return fail(errorCode, "onWrite");
	}
	else if (close) {
		_tcpStream.socket().shutdown(net::ip::tcp::socket::shutdown_send, errorCode);
		return;
	}
	else {
		doRead();
	}
}

void HttpSession::fail(const beast::error_code errorCode, const char* what) {
	if (errorCode == net::error::operation_aborted) {
		return;
	}
	else {
		std::cerr << what << ": " << errorCode.message() << '\n';
	}
}
