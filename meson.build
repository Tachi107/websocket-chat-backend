project(
	'Websocket-chat-backend',
	'cpp',
	default_options: [
		'cpp_std=c++2a',
		'c_std=c11',
		'b_ndebug=if-release',
		'b_lto=true'
	]
)

if meson.get_compiler('cpp').get_id() == 'clang' and get_option('buildtype').startswith('debug')
	cpp_args = ['-gdwarf-4', '-glldb', '-fstandalone-debug', '-fdebug-info-for-profiling', '-fdebug-macro', '-fdebug-default-version=4', '-fno-omit-frame-pointer', '-fno-optimize-sibling-calls', '-Wall', '-Wextra', '-Weverything', '-Wpedantic', '-Wno-unused-parameter', '-Wno-c++98-compat', '-Wno-c++98-compat-pedantic', '-Wno-missing-prototypes', '-Wno-global-constructors', '-Wno-exit-time-destructors', '-Wno-shadow', '-Wno-documentation', '-Wno-documentation-unknown-command']
else
	cpp_args = []
endif

if not meson.is_cross_build()
	add_project_arguments('-march=native', language: 'cpp')
endif

src = [
	'websocket-chat/main.cpp',
	'websocket-chat/http_session.cpp',
	'websocket-chat/listener.cpp',
	'websocket-chat/shared_state.cpp',
	'websocket-chat/websocket_session.cpp'
]

executable(
	'Websocket-chat',
	sources: src,
	include_directories: 'websocket-chat',
	dependencies: [
		dependency('boost'),
		dependency('threads')
	],
	cpp_args: cpp_args
)
