#pragma once
#include <ostream>
#include <vector>

enum LogLevel
{
	INFO = 0,
	DEBUG = 1,
	WARNING = 2,
	ERROR = 3,
	FATAL = 4
};

class LogStream
{
	friend class SimpleLogger;
public:
	LogStream(LogLevel logLevel, std::ostream& os, const std::string& fmt = "[$l] $m");
	~LogStream() = default;

	template<typename T>
	const LogStream& operator<<(const T& buf) const;
	const LogStream& Initialize(const char* file, const char* line, const char* funcsig, LogLevel level);

private:
	LogLevel level;
	std::ostream& ostream;

	std::string format;
};

class SimpleLogger
{
public:
	SimpleLogger() = default;
	~SimpleLogger() = default;

	void Add(LogStream& logStream);
	const SimpleLogger& Initialize(const char* file, const char* line, const char* funcsig, LogLevel level);

	template<typename T>
	const SimpleLogger& operator<<(const T& buf) const;

	static SimpleLogger DefaultLogger;
private:
	std::vector<LogStream> logStreams;
};

