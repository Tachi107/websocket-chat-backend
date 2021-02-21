#pragma once

#include <websocket-chat/common.hpp>
#include <websocket-chat/shared_state.hpp>
#include <iostream>

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession> {
public:
	WebsocketSession(net::ip::tcp::socket socket, const std::shared_ptr<SharedState>& state) 
		: _websocketStream(std::move(socket)), _state(state) {}
	~WebsocketSession() {
		// Elimino questa sessione dalla lista delle sessioni attive
		_state->leave(*this);
	}

// Funzione che prende la richiesta HTTP di upgrade e accetta lo scambio dati ws
template<typename Body, typename Allocator>
void run(beast::http::request<Body, beast::http::basic_fields<Allocator>> request) {
	_websocketStream.async_accept(
		request,
		[self = shared_from_this()](boost::system::error_code errorCode) {
			self->onAccept(errorCode);
		}
	);
}

void send(const std::shared_ptr<const std::string>& sharedString) {
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
		[sharedPtr = shared_from_this()](boost::system::error_code errorCode, std::size_t bytes) {
			sharedPtr->onWrite(errorCode, bytes);
		}
	);
}

private:
void onAccept(boost::system::error_code errorCode) {
	if (errorCode) {
		return fail(errorCode, "onAccept");
	}
	else {
		// Aggiungo la sessione ws alla lista delle sessioni attive
		// Questo mi serve per sapere chi è connesso e
		// broadcastare i messaggi
		_state->join(*this);
		// E mi metto in attesa di messaggi per poi leggerli, in modo asincrono
		_websocketStream.async_read(
			_buffer,
			[sharedPtr = shared_from_this()](boost::system::error_code errorCode, std::size_t bytes) {
				sharedPtr->onRead(errorCode, bytes);
			}
		);
	}
}

void onRead(boost::system::error_code errorCode, std::size_t /*bytes*/) {
	if (errorCode) {
		return fail(errorCode, "onRead");
	}
	else {
		// Invia il messaggio a tutti i partecipanti
		_state->send(beast::buffers_to_string(_buffer.data()));
		// Una volta usato, svuoto il buffer
		_buffer.consume(_buffer.size());
		// Mi rimetto in attesa di altri messaggi
		_websocketStream.async_read(
			_buffer,
			[sharedPtr = shared_from_this()](boost::system::error_code errorCode, std::size_t bytes) {
				sharedPtr->onRead(errorCode, bytes);
			}
		);
	}
}

void onWrite(boost::system::error_code errorCode, std::size_t /*bytes*/) {
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
				[sharedPtr = shared_from_this()](boost::system::error_code errorCode, std::size_t bytes) {
					sharedPtr->onWrite(errorCode, bytes);
				}
			);
		}
	}
}

void fail(boost::system::error_code errorCode, const char* what) {
	if (errorCode == net::error::operation_aborted ||
		errorCode == beast::websocket::error::closed) {
		return;
	}
	else {
		std::cerr << what << ": " << errorCode.message() << '\n';
	}
}

private:
	beast::flat_buffer _buffer;
	beast::websocket::stream<net::ip::tcp::socket> _websocketStream; // Contiene stato ws
	std::shared_ptr<SharedState> _state;
	std::vector<std::shared_ptr<const std::string>> _messageQueue;
};
