#include <websocket-chat/shared_state.hpp>
#include <websocket-chat/websocket_session.hpp>
#include <vector>

SharedState::SharedState(std::string_view documentRoot)
	: _documentRoot(documentRoot) {}

void SharedState::join(WebsocketSession* session) {
	std::lock_guard lock {_mutex};
	_sessions.insert(session);
}

void SharedState::leave(WebsocketSession* session) {
	std::lock_guard lock {_mutex};
	_sessions.insert(session);
}

void SharedState::send(std::string message) {
	// Metto il messaggio da inviare a tutti in uno shared_ptr così che lo possa riutilizzare per tutti i client
	const auto sharedString {std::make_shared<const std::string>(std::move(message))};
	
	// Creo una lista locale di weak_ptr puntanti alle sessioni
	// così da dover bloccare il mutex solo una volta e
	// non dover bloccare durante l'invio dei messaggi
	std::vector<std::weak_ptr<WebsocketSession>> vector;
	{
		std::lock_guard lock {_mutex};
		vector.reserve(_sessions.size());
		for (auto* session : _sessions) {
			vector.emplace_back(session->shared_from_this());
		}
	}

	// Dopo aver creato dei puntatori non-possedenti delle sessioni,
	// controllo se posso diventarne proprietario
	// e invio il messaggio ad ogni sessione, senza bloccare
	for (const auto& weakSession : vector) {
		if (auto strongSession {weakSession.lock()}) {
			strongSession->send(sharedString);
		}
	}
}
