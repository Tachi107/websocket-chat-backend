#pragma once

#include <string>
#include <unordered_set>
#include <memory>
#include <mutex>

class WebsocketSession;

class SharedState {
public:
	SharedState(std::string_view documentRoot);

	void join(WebsocketSession* session);
	void leave(WebsocketSession* session);
	void send(std::string message);
	void sendToGroup(std::string message);

	[[nodiscard]] const std::string& documentRoot() const noexcept {
		return _documentRoot;
	}

private:
	const std::string _documentRoot;
	std::unordered_set<WebsocketSession*> _sessions;
};
