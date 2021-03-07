#include <websocket-chat/websocket_session.hpp>
#include <iostream>

WebsocketSession::WebsocketSession(net::ip::tcp::socket&& socket, const std::shared_ptr<SharedState>& state)
	: _websocketStream(std::move(socket)), _state(state) {}

WebsocketSession::~WebsocketSession() {
	// Elimino questa sessione dalla lista delle sessioni attive
	_state->leave(this);
}

//void WebsocketSession::send(const std::shared_ptr<const std::string>& sharedString) {
//	// Post our work to the strand, this ensures
//	// that the members of `this` will not be
//	// accessed concurrently.
//	net::post(
//		_websocketStream.get_executor(),
//		beast::bind_front_handler(
//			&WebsocketSession::onSend,
//			shared_from_this(),
//			sharedString
//		)
//	);
//}

void WebsocketSession::onAccept(beast::error_code errorCode)  {
	if (errorCode) {
		return fail(errorCode, "onAccept");
	}
	else {
		// Aggiungo la sessione ws alla lista delle sessioni attive
		// Questo mi serve per sapere chi è connesso e
		// broadcastare i messaggi
		_state->join(this);
		// E mi metto in attesa di messaggi per poi leggerli, in modo asincrono
		_websocketStream.async_read(
			_buffer,
			beast::bind_front_handler(
				&WebsocketSession::onRead,
				shared_from_this()
			)
		);
	}
}

void WebsocketSession::onRead(beast::error_code errorCode, std::size_t /*bytes*/) {
	if (errorCode) {
		return fail(errorCode, "onRead");
	}
	else {
		//simdjson::dom::element parsed {_jsonParser.parse(beast::buffers_to_string(_buffer.data()))};
		//_userId = parsed["user-id"];
		//_groupId = parsed["group-id"];

		//_state.sendToUser(userId, message);
		//_state.sendToGroup(groupId, message);
		// Invia il messaggio a tutti i partecipanti
		_state->send(beast::buffers_to_string(_buffer.data()));
		// Una volta usato, svuoto il buffer
		_buffer.consume(_buffer.size());
		// Mi rimetto in attesa di altri messaggi
		_websocketStream.async_read(
			_buffer,
			beast::bind_front_handler(
				&WebsocketSession::onRead, 
				shared_from_this()
			)
		);
	}
}

void WebsocketSession::onWrite(beast::error_code errorCode, std::size_t /*bytes*/) {
	if (errorCode) {
		return fail(errorCode, "onWrite");
	}
	else {
		// Una volta aver scritto il messaggio lo elimino dalla coda
		_messageQueue.erase(_messageQueue.begin());
		// E continuerò ad inviare messaggi fino a che la coda non sarà vuota
		if (!_messageQueue.empty()) {
			_websocketStream.async_write(
				net::buffer(*_messageQueue.front()),
				beast::bind_front_handler(
					&WebsocketSession::onWrite,
					shared_from_this()
				)
			);
		}
	}
}

void WebsocketSession::send(const std::shared_ptr<const std::string>& sharedString) {
	// Metto il messaggio in coda
	_messageQueue.push_back(sharedString);
	// Controllo se sto già inviando qualcosa a qualcuno,
	// in quanto posso inviare solo un messaggio alla volta.
	// Se sto già inviando termino la funzione, aspettando che
	// la scrittura corrente termini
	if (_messageQueue.size() > 1) {
		return;
	}
	// Trasformo la string in buffer così da poterla inviare,
	// dopodiché mi rimetto in loop per scrivere 
	_websocketStream.async_write(
		net::buffer(*_messageQueue.front()),
		beast::bind_front_handler(
			&WebsocketSession::onWrite, 
			shared_from_this()
		)
	);
}

void WebsocketSession::fail(const beast::error_code errorCode, const char* what) {
	if (errorCode == net::error::operation_aborted ||
		errorCode == beast::websocket::error::closed) {
		return;
	}
	else {
		std::cerr << what << ": " << errorCode.message() << '\n';
	}
}
