#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace apr_system {

/**
 * @brief central logging utility for the APR project system
 *
 * provides structured logging with different levels and outputs.
 * wraps spdlog functionality for consistent logging across all components.
 */
class Logger {
public:
  enum class Level {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5
  };

  /**
   * @brief initialize the logging system
   * @param log_level minimum log level to output
   * @param enable_file_logging whether to enable file logging
   * @param log_file_path path for log file (if file logging enabled)
   * @param max_file_size maximum size of log file before rotation
   * @param max_files maximum number of log files to keep
   */
  static void
  initialize(Level log_level = Level::INFO, bool enable_file_logging = true,
             const std::string &log_file_path = "logs/apr_system.log",
             size_t max_file_size = 1048576 * 10, // 10mb
             size_t max_files = 3);

  /**
   * @brief get the logger instance
   * @return shared pointer to the logger
   */
  static std::shared_ptr<spdlog::logger> get();

  /**
   * @brief get component-specific logger with unique color
   * @param component_name component name (sbfl, parser, mutator, etc.)
   * @return shared pointer to component logger
   */
  static std::shared_ptr<spdlog::logger>
  getComponentLogger(const std::string &component_name);

  /**
   * @brief initialize component loggers with unique colors
   */
  static void initializeComponentLoggers();

  /**
   * @brief log a pipeline step with context
   * @param component component name (e.g., "sbfl", "mutator")
   * @param step step description
   * @param details additional details (optional)
   */
  static void logPipelineStep(const std::string &component,
                              const std::string &step,
                              const std::string &details = "");

  /**
   * @brief log component initialization
   * @param component_name name of the component being initialized
   */
  static void logComponentInit(const std::string &component_name);

  /**
   * @brief log an error with context
   * @param component component where error occurred
   * @param error_msg error message
   * @param context additional context (optional)
   */
  static void logError(const std::string &component,
                       const std::string &error_msg,
                       const std::string &context = "");

  /**
   * @brief log performance metrics
   * @param operation operation name
   * @param duration_ms duration in milliseconds
   * @param additional_info additional information (optional)
   */
  static void logPerformance(const std::string &operation, double duration_ms,
                             const std::string &additional_info = "");

  /**
   * @brief set log level dynamically
   * @param level new log level
   */
  static void setLevel(Level level);

  /**
   * @brief shutdown the logging system
   */
  static void shutdown();

private:
  static std::shared_ptr<spdlog::logger> logger_;
  static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>
      component_loggers_;
  static bool initialized_;

  static spdlog::level::level_enum convertLevel(Level level);
};

// Convenience macros for common logging patterns
#define LOG_TRACE(...) apr_system::Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) apr_system::Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...) apr_system::Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...) apr_system::Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...) apr_system::Logger::get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) apr_system::Logger::get()->critical(__VA_ARGS__)

// Specialized macros for APR system
#define LOG_PIPELINE_STEP(component, step, ...)                                \
  apr_system::Logger::logPipelineStep(component, step, ##__VA_ARGS__)
#define LOG_COMPONENT_INIT(component)                                          \
  apr_system::Logger::logComponentInit(component)
#define LOG_APR_ERROR(component, msg, ...)                                     \
  apr_system::Logger::logError(component, msg, ##__VA_ARGS__)
#define LOG_PERFORMANCE(operation, duration_ms, ...)                           \
  apr_system::Logger::logPerformance(operation, duration_ms, ##__VA_ARGS__)

// Component-specific logging macros with unique colors
#define LOG_COMPONENT_INFO(component, ...)                                     \
  apr_system::Logger::getComponentLogger(component)->info(__VA_ARGS__)
#define LOG_COMPONENT_WARN(component, ...)                                     \
  apr_system::Logger::getComponentLogger(component)->warn(__VA_ARGS__)
#define LOG_COMPONENT_ERROR(component, ...)                                    \
  apr_system::Logger::getComponentLogger(component)->error(__VA_ARGS__)
#define LOG_COMPONENT_DEBUG(component, ...)                                    \
  apr_system::Logger::getComponentLogger(component)->debug(__VA_ARGS__)

} // namespace apr_system
