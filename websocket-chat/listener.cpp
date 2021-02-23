#include <websocket-chat/listener.hpp>
#include <websocket-chat/http_session.hpp>
#include <iostream>

Listener::Listener(net::io_context& ioContext, net::ip::tcp::endpoint endpoint, const std::shared_ptr<SharedState>& state)
	: _ioContext(ioContext), _acceptor(ioContext), _state(state) {
	
	beast::error_code errorCode;

	_acceptor.open(endpoint.protocol(), errorCode);
	if (errorCode) {
		fail(errorCode, "_acceptor.open");
		return;
	}

	_acceptor.set_option(net::socket_base::reuse_address(true), errorCode);
	if (errorCode) {
		fail(errorCode, "_acceptor.set_option");
		return;
	}

	_acceptor.bind(endpoint, errorCode);
	if (errorCode) {
		fail(errorCode, "_acceptor.bind");
		return;
	}

	_acceptor.listen(net::socket_base::max_listen_connections, errorCode);
	if (errorCode) {
		fail(errorCode, "_acceptor.listen");
		return;
	}
}

void Listener::run() {
	_acceptor.async_accept(
		net::make_strand(_ioContext),
		// Per estendere la vita dello shared_ptr che ha chiamato run() devo chiamare shared_from_this()
		beast::bind_front_handler(
			&Listener::onAccept,
			shared_from_this()
		)
	);
}

void Listener::onAccept(beast::error_code errorCode, net::ip::tcp::socket socket) {
	if (errorCode) {
		return fail(errorCode, "onAccept");
	}
	else {
		std::make_shared<HttpSession>(
			std::move(socket),
			_state
		)->run();
	}
	// Una volta aver accettato una richiesta,
	// richiamo onAccept in modo asincrono.
	// Questo fa sì che onAccept non termini mai,
	// perché una vota accettata una connessione
	// una funzione asincrona si mette in attesa di accettarne un'altra,
	// creando un loop senza loop
	_acceptor.async_accept(
		net::make_strand(_ioContext),
		beast::bind_front_handler(
			&Listener::onAccept,
			shared_from_this()
		)
	);
}

void Listener::fail(const beast::error_code errorCode, const char* what) {
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
