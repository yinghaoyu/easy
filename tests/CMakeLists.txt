add_executable(test_log test_log.cc)
target_link_libraries(test_log easy_base)

add_executable(test_fiber test_fiber.cc)
target_link_libraries(test_fiber easy_base)

add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread easy_base)

add_executable(test_scheduler test_scheduler.cc)
target_link_libraries(test_scheduler easy_base)

add_executable(test_iomanager test_iomanager.cc)
target_link_libraries(test_iomanager easy_base)

add_executable(test_env test_env.cc)
target_link_libraries(test_env easy_base)

add_executable(test_config test_config.cc)
target_link_libraries(test_config easy_base)

add_executable(test_hook test_hook.cc)
target_link_libraries(test_hook easy_base)

add_executable(test_address test_address.cc)
target_link_libraries(test_address easy_net easy_base)

add_executable(test_socket test_socket.cc)
target_link_libraries(test_socket easy_net easy_base)

add_executable(test_tcp_server test_tcp_server.cc)
target_link_libraries(test_tcp_server easy_net easy_base)
