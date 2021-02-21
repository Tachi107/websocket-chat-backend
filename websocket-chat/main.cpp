#include <websocket-chat/http_session.hpp>
#include <websocket-chat/listener.hpp>
#include <websocket-chat/shared_state.hpp>
#include <websocket-chat/websocket_session.hpp>

// address, port, documentRoot
int main(int argc, char* argv[]) {
	net::io_context ioContext;

	std::make_shared<Listener>(
		ioContext,
		net::ip::tcp::endpoint{net::ip::make_address(argv[1]), static_cast<unsigned short>(std::atoi(argv[2]))},
		std::make_shared<SharedState>(argv[3])
	)->run();

	ioContext.run();
}
