#pragma once

typedef enum LogLevel {
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_DEBUG = 4,
	LOG_LEVEL_TRACE = 5
};

#define ASSERT(expr) if(expr) {} else { ReportAssertionFailure(#expr, "", __FILE__, __LINE__);}
#define ASSERT_MSG(expr, message) if(expr) {} else { ReportAssertionFailure(#expr, message, __FILE__, __LINE__); __debugbreak(); }

#define LOG_FATAL(message, ...) LogOutput(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define LOG_ERROR(message, ...) LogOutput(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#define LOG_WARN(message, ...) LogOutput(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#define LOG_INFO(message, ...) LogOutput(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#define LOG_DEBUG(message, ...) LogOutput(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#define LOG_TRACE(message, ...) LogOutput(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);

bool InitializeLogger();

void ShutdownLogger();

void LogOutput(LogLevel level, const char* message, ...);

void ReportAssertionFailure(const char* expression, const char* message, const char* file, int line);