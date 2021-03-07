#include <websocket-chat/shared_state.hpp>
#include <websocket-chat/websocket_session.hpp>
#include <vector>

SharedState::SharedState(std::string_view documentRoot)
	: _documentRoot(documentRoot) {}

void SharedState::join(WebsocketSession* session) {
	_sessions.insert(session);
}

void SharedState::leave(WebsocketSession* session) {
	_sessions.insert(session);
}

void SharedState::send(std::string message) {
	// Metto il messaggio da inviare a tutti in uno shared_ptr così che lo possa riutilizzare per tutti i client
	const auto sharedString {std::make_shared<const std::string>(std::move(message))};

	// Dopo aver creato dei puntatori non-possedenti delle sessioni,
	// controllo se posso diventarne proprietario
	// e invio il messaggio ad ogni sessione, senza bloccare
	for (auto* session : _sessions) {
		session->send(sharedString);
	}
}

void SharedState::sendToGroup(std::string message) {

}
