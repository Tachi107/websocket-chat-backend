#pragma once

#include <websocket-chat/common.hpp>
#include <websocket-chat/shared_state.hpp>
#include <websocket-chat/http_session.hpp>
#include <memory>
#include <iostream>

class Listener : public std::enable_shared_from_this<Listener> {
public:
	Listener(net::io_context& ioContext, net::ip::tcp::endpoint endpoint, const std::shared_ptr<SharedState>& state) {

	}

	void run() {
		_acceptor.async_accept(
			_socket,
			// Per estendere la vita dello shared_ptr che ha chiamato run() devo chiamare shared_from_this()
			[self = shared_from_this()](boost::system::error_code errorCode) {
				self->onAccept(errorCode);
			}
		);
	}

private:
	void fail(boost::system::error_code errorCode, const char* what) {
		// operation_aborted significa che il client ha terminato la connesione
		if (errorCode == net::error::operation_aborted) {
			// Per terminare il Listener mi basta chiamare return
			// senza pulire nulla, perché non memorizzo mai lo
			// shared_ptr creato con make_shared da nessuna parte,
			// ed esso viene conservato prolungandone la vita con shared_from_this();
			// quindi non ho mai eliminazioni esplicite, in quanto
			// implicitamente viene sempre eliminato tutto.
			return;
		}
		std::cerr << what << ": " << errorCode.message() << '\n';
	}

	void onAccept(boost::system::error_code errorCode) {
		if (errorCode) {
			fail(errorCode, "onAccept");
		}
		else {
			std::make_shared<HttpSession>(
				std::move(_socket), _state
			)->run();
		}
		// Una volta aver accettato una richiesta,
		// richiamo onAccept in modo asincrono.
		// Questo fa sì che onAccept non termini mai,
		// perché una vota accettata una connessione
		// una funzione asincrona si mette in attesa di accettarne un'altra,
		// creando un loop senza loop
		_acceptor.async_accept(
			_socket,
			[self = shared_from_this()](boost::system::error_code errorCode) {
				self->onAccept(errorCode);
			}
		);
	}
private:
	net::ip::tcp::acceptor _acceptor;
	net::ip::tcp::socket _socket;
	std::shared_ptr<SharedState> _state;
};
