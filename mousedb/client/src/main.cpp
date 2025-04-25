#include <print>

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

void somewhere_else() {
    auto console_sink =
        quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
    quill::Logger *logger =
        quill::Frontend::create_or_get_logger("root", std::move(console_sink));
    LOG_INFO(logger, "Hello from somewhere else {} {} {}", "arg", 232, "stuff");
};

int main(int argc, char **argv) {
    quill::Backend::start();
    auto console_sink =
        quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
    quill::Logger *logger =
        quill::Frontend::create_or_get_logger("root", std::move(console_sink));
    LOG_INFO(logger, "Hello from main {} {} {}", "arg", 232, "stuff");
    somewhere_else();
}
