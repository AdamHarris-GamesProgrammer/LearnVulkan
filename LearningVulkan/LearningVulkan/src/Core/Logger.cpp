#include "Logger.h"

#include <stdio.h>
#include <string>
#include <stdarg.h>

bool InitializeLogger() {
	//TODO: Create a queue here that we can add to across threads, this will allows us to output to a log file correctly

	return true;
}

void ShutdownLogger() {
	//TODO: Cleanup of queued logs
}

void LogOutput(LogLevel level, const char* message, ...) {
	const char* levelString[6] = { "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: " };
	
	char outputMessage[32000];
	memset(outputMessage, 0, sizeof(outputMessage));

	//Format message
	char* arg_ptr;
	va_start(arg_ptr, message);
	vsnprintf(outputMessage, 32000, message, arg_ptr);
	va_end(arg_ptr);

	char outputMessage2[32000];
	sprintf_s(outputMessage2, "%s%s\n", levelString[level], outputMessage);

	//TODO: coloured output.
	printf("%s", outputMessage2);
}

void ReportAssertionFailure(const char* expression, const char* message, const char* file, int line) {
	LogOutput(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: %s, in file: %s, line: %d", expression, message, file, line);
}