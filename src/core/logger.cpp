#include "logger.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace apr_system {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
std::unordered_map<std::string, std::shared_ptr<spdlog::logger> > Logger::component_loggers_;
bool Logger::initialized_ = false;

void Logger::initialize(
    Level log_level,
    bool enable_file_logging,
    const std::string& log_file_path,
    size_t max_file_size,
    size_t max_files
) {
    if (initialized_) {
        return; // already initialized
    }

    try {
        std::vector<spdlog::sink_ptr> sinks;

        // console sink with color
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(convertLevel(log_level));
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        sinks.push_back(console_sink);

        // file sink (if enabled)
        if (enable_file_logging) {
            // create log directory if it doesn't exist
            std::filesystem::path log_path(log_file_path);
            std::filesystem::create_directories(log_path.parent_path());

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file_path, max_file_size, max_files);
            file_sink->set_level(spdlog::level::trace); // log everything to file
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v");
            sinks.push_back(file_sink);
        }

        // create multi-sink logger
        logger_ = std::make_shared<spdlog::logger>("apr_system", sinks.begin(), sinks.end());
        logger_->set_level(convertLevel(log_level));
        logger_->flush_on(spdlog::level::warn);

        // register the logger
        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);

        initialized_ = true;

        LOG_INFO("APR project system logging initialized");
        LOG_INFO("log level: {}", spdlog::level::to_string_view(convertLevel(log_level)));
        if (enable_file_logging) {
            LOG_INFO("file logging enabled: {}", log_file_path);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        // fallback to basic console logger
        logger_ = spdlog::stdout_color_mt("apr_system_fallback");
        initialized_ = true;
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!initialized_) {
        // auto-initialize with default settings if not done explicitly
        initialize();
    }
    return logger_;
}

void Logger::logPipelineStep(const std::string& component, const std::string& step, const std::string& details) {
    if (details.empty()) {
        LOG_INFO("[{}] {}", component, step);
    } else {
        LOG_INFO("[{}] {} - {}", component, step, details);
    }
}

void Logger::logComponentInit(const std::string& component_name) {
    LOG_DEBUG("initializing component: {}", component_name);
}

void Logger::logError(const std::string& component, const std::string& error_msg, const std::string& context) {
    if (context.empty()) {
        LOG_ERROR("[{}] {}", component, error_msg);
    } else {
        LOG_ERROR("[{}] {} (Context: {})", component, error_msg, context);
    }
}

void Logger::logPerformance(const std::string& operation, double duration_ms, const std::string& additional_info) {
    if (additional_info.empty()) {
        LOG_INFO("{} completed in {:.2f}ms", operation, duration_ms);
    } else {
        LOG_INFO("{} completed in {:.2f}ms - {}", operation, duration_ms, additional_info);
    }
}

void Logger::setLevel(Level level) {
    if (logger_) {
        logger_->set_level(convertLevel(level));
        LOG_INFO("log level changed to: {}", spdlog::level::to_string_view(convertLevel(level)));
    }
}

void Logger::shutdown() {
    if (logger_) {
        LOG_INFO("shutting down APR project system logging");
        logger_->flush();
        spdlog::shutdown();
        initialized_ = false;
    }
}

spdlog::level::level_enum Logger::convertLevel(Level level) {
    switch (level) {
        case Level::TRACE: return spdlog::level::trace;
        case Level::DEBUG: return spdlog::level::debug;
        case Level::INFO: return spdlog::level::info;
        case Level::WARN: return spdlog::level::warn;
        case Level::ERROR: return spdlog::level::err;
        case Level::CRITICAL: return spdlog::level::critical;
        default: return spdlog::level::info;
    }
}

std::shared_ptr<spdlog::logger> Logger::getComponentLogger(const std::string& component_name) {
    if (component_loggers_.find(component_name) == component_loggers_.end()) {
        initializeComponentLoggers();
    }

    auto it = component_loggers_.find(component_name);
    if (it != component_loggers_.end()) {
        return it->second;
    }

    // fallback to main logger if component logger not found
    return get();
}

void Logger::initializeComponentLoggers() {
    if (!initialized_) {
        initialize(); // ensure main logger is initialized first
    }

    try {
        // define component colors using spdlog color codes
        std::unordered_map<std::string, std::string> component_colors = {
            {"orchestrator", "\033[37m"}, // white
            {"cli", "\033[37m"},           // white
            {"sbfl", "\033[32m"},         // green
            {"parser", "\033[34m"},       // blue
            {"mutator", "\033[35m"},      // magenta
            {"prioritizer", "\033[33m"},  // yellow
            {"validator", "\033[36m"},    // cyan
        };

        for (const auto& [component, color] : component_colors) {
            // create console sink with component-specific color pattern
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] " + color + "[" + component + "]" + "\033[0m %v");

            // create logger for this component
            auto component_logger = std::make_shared<spdlog::logger>(component, console_sink);
            component_logger->set_level(logger_->level());
            component_logger->flush_on(spdlog::level::warn);

            component_loggers_[component] = component_logger;
        }

        LOG_DEBUG("component loggers initialized with unique colors");
    }
    catch (const std::exception& e) {
        LOG_ERROR("failed to initialize component loggers: {}", e.what());
    }
}

} // namespace apr_system
