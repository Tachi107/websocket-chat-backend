#include <websocket-chat/listener.hpp>
#include <websocket-chat/shared_state.hpp>
#include <thread>
#include <iostream>

int main(int argc, char* argv[]) {
	if (argc != 5) {
		std::cerr << "Usage: Websocket-chat <address> <port> <document root> <threads>\nExample: Websocket-chat 0.0.0.0 10781 . 2\n";
		return EXIT_FAILURE;
	}
	const net::ip::address address {net::ip::make_address(argv[1])};
	const unsigned short port = std::atoi(argv[2]);
	const char* documentRoot {argv[3]};
	const std::uint32_t threadCount {std::max<uint32_t>(1, std::atoi(argv[4]))};

	net::io_context ioContext;
	
	std::make_shared<Listener>(
		ioContext,
		net::ip::tcp::endpoint{address, port},
		std::make_shared<SharedState>(documentRoot)
	)->run();

	net::signal_set signals {ioContext, SIGINT, SIGTERM};
	signals.async_wait(
		[&ioContext](const beast::error_code& /*unused*/, int /*unused*/) {
			ioContext.stop();
		}
	);

	std::vector<std::jthread> threads;
	threads.reserve(threadCount - 1);
	for (auto i {threadCount - 1}; i > 0; --i) {
		threads.emplace_back(
			[&ioContext](){
				ioContext.run();
			}
		);
	}
	ioContext.run();
}
