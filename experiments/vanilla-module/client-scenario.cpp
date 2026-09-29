#define SDL_MAIN_HANDLED
#define ANYBANDUI_CLIENT_TEST
#define main frontend_cpp_client_test
#include "test_session_ui.cpp"
#undef main
extern "C" int frontend_client_test(int argc, char **argv) {
    return frontend_cpp_client_test(argc, argv);
}
