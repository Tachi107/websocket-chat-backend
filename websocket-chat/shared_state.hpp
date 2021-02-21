#pragma once

#include <string>
#include <unordered_set>
#include <memory>
#include <websocket-chat/websocket_session.hpp>

class SharedState {
public:
	SharedState(std::string_view documentRoot);

	void join(WebsocketSession& session) {
		_sessions.insert(&session);
	}

	void leave(WebsocketSession& session) {
		_sessions.erase(&session);
	}

	void send(std::string message) {
		const auto sharedString {std::make_shared<const std::string>(std::move(message))};
		for (auto* session : _sessions) {
			session->send(sharedString);
		}
	}

	[[nodiscard]] const std::string& documentRoot() const noexcept {
		return _documentRoot;
	}

private:
	std::string _documentRoot;
	std::unordered_set<WebsocketSession*> _sessions;
};
