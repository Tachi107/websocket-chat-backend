#pragma once

#include <websocket-chat/common.hpp>
#include <websocket-chat/shared_state.hpp>

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
	HttpSession(net::ip::tcp::socket socket, const std::shared_ptr<SharedState>& state)
		: _socket(std::move(socket)), _state(state) {}
	
	void run() {
		beast::http::async_read(
			_socket,
			_buffer,
			_request,
			[self = shared_from_this()](boost::system::error_code errorCode, std::size_t bytes) {
				self->onRead(errorCode, bytes);				
			}
		);
	}

private:
	void onRead(boost::system::error_code errorCode, std::size_t /*bytes*/) {
		// Client closed the connection
		if (errorCode == beast::http::error::end_of_stream) {
			_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, errorCode);
			return;
		}
		else if (errorCode) {
			return fail(errorCode, "onRead");
		}
		// I websocket vengono inizializzati con un'apposita richiesta HTTP
		else if (beast::websocket::is_upgrade(_request)) {
			// Creo quindi una sessione Websocket spostando il socket
			// dalla sessione HTTP a quella WS, insieme alla richiesta,
			// così che la WebsocketSession ne diventi padrona
			std::make_shared<WebsocketSession>(
				std::move(_socket),
				_state
			)->run(std::move(_request));
			// Ritorno, così che la sessione HTTP venga automaticamente distrutta
			return;
		}
		else {
			handleRequest(
				_state->documentRoot(),
				std::move(_request),
				[this](auto&& response) {
					using ResponseType = typename std::decay<decltype(response)>::type;
					auto sharedPtr {std::make_shared<ResponseType>(std::move(response))};
					beast::http::async_write(
						this->_socket,
						*sharedPtr,
						[self = shared_from_this(), sharedPtr](boost::system::error_code errorCode, std::size_t bytes) {
							self->onWrite(errorCode, bytes, sharedPtr->need_eof());
						}
					);
				}
			);
		}
	}

	void onWrite(boost::system::error_code errorCode, std::size_t /*bytes*/, bool close) {
		if (errorCode) {
			return fail(errorCode, "onWrite");
		}
		else if (close) {
			_socket.shutdown(net::ip::tcp::socket::shutdown_send, errorCode);
			return;
		}
		else {
			// Una volta che leggo la richiesta la svuoto
			_request = {};
			// E mi rimetto a leggere richieste
			beast::http::async_read(
				_socket,
				_buffer,
				_request,
				[self = shared_from_this()](boost::system::error_code errorCode, std::size_t bytes) {
					self->onRead(errorCode, bytes);
				}
			);
		}
	}

	template<typename Body, typename Allocator, typename Send>
void handle_request(
	std::string_view doc_root,
	beast::http::request<Body, beast::http::basic_fields<Allocator>>&& req,
	Send&& send
) {
	// Returns a bad request response
	const auto bad_request =
	[&req](std::string_view why) {
		beast::http::response<beast::http::string_body> response {beast::http::status::bad_request, req.version()};
		response.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		response.set(beast::http::field::content_type, "text/html");
		response.keep_alive(req.keep_alive());
		response.body() = std::string{why};
		response.prepare_payload();
		return response;
	};

	// Returns a not found response
	const auto not_found =
	[&req](std::string_view target)
	{
		beast::http::response<beast::http::string_body> response{beast::http::status::not_found, req.version()};
		response.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		response.set(beast::http::field::content_type, "text/html");
		response.keep_alive(req.keep_alive());
		response.body() = "The resource '" + std::string{target} + "' was not found.";
		response.prepare_payload();
		return response;
	};

	// Returns a server error response
	const auto server_error =
	[&req](std::string_view what)
	{
		beast::http::response<beast::http::string_body> response{beast::http::status::internal_server_error, req.version()};
		response.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		response.set(beast::http::field::content_type, "text/html");
		response.keep_alive(req.keep_alive());
		response.body() = "An error occurred: '" + std::string{what} + "'";
		response.prepare_payload();
		return response;
	};

	// Make sure we can handle the method
	if (req.method() != beast::http::verb::get &&
		req.method() != beast::http::verb::head)
		return send(bad_request("Unknown HTTP-method"));

	// Request path must be absolute and not contain "..".
	if (req.target().empty() ||
		req.target()[0] != '/' ||
		req.target().find("..") != std::string_view::npos)
		return send(bad_request("Illegal request-target"));

	// Build the path to the requested file
	std::string path = path_cat(doc_root, req.target());
	if (req.target().back() == '/')
		path.append("index.html");

	// Attempt to open the file
	beast::error_code ec;
	beast::http::file_body::value_type body;
	body.open(path.c_str(), beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (ec == boost::system::errc::no_such_file_or_directory)
		return send(not_found(req.target()));

	// Handle an unknown error
	if (ec) {
		return send(server_error(ec.message()));
	}

	// Cache the size since we need it after the move
	const auto size = body.size();

	// Respond to HEAD request
	if (req.method() == beast::http::verb::head)
	{
		beast::http::response<beast::http::empty_body> response{beast::http::status::ok, req.version()};
		response.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		response.set(beast::http::field::content_type, mime_type(path));
		response.content_length(size);
		response.keep_alive(req.keep_alive());
		return send(std::move(response));
	}

	// Respond to GET request
	beast::http::response<beast::http::file_body> response{
		std::piecewise_construct,
		std::make_tuple(std::move(body)),
		std::make_tuple(beast::http::status::ok, req.version())};
	response.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
	response.set(beast::http::field::content_type, mime_type(path));
	response.content_length(size);
	response.keep_alive(req.keep_alive());
	return send(std::move(response));
}
private:
	net::ip::tcp::socket _socket;
	beast::flat_buffer _buffer;
	std::shared_ptr<SharedState> _state;
	beast::http::request<beast::http::string_body> _request;
};
