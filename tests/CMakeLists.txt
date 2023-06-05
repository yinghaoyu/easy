add_executable(test_log test_log.cc)
target_link_libraries(test_log easy_base)

add_executable(test_coroutine test_coroutine.cc)
target_link_libraries(test_coroutine easy_base)

add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread easy_base)

add_executable(test_scheduler test_scheduler.cc)
target_link_libraries(test_scheduler easy_base)
