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

	typedef std::ostream& (*manipulator)(std::ostream&);
	const LogStream& operator<<(manipulator pf);

	const LogStream& Initialize(const char* time, const char* file, const char* line, const char* funcsig, LogLevel level);
private:
	LogLevel level;
	std::ostream& ostream;

	std::string format;

	bool initialized;

	size_t fmtPtr;
};

class SimpleLogger
{
public:
	SimpleLogger() = default;
	~SimpleLogger() = default;

	void Add(LogStream& logStream);
	const SimpleLogger& Initialize(const char* time, const char* file, const char* line, const char* funcsig, LogLevel level);

	template<typename T>
	const SimpleLogger& operator<<(const T& buf) const;

	const SimpleLogger& operator<<(LogStream::manipulator pf) const;
private:
	std::vector<LogStream> logStreams;
};

#define ADD_LOGGER(logger) (DefaultLogger.Add(logger));
#define LOG(level) (DefaultLogger.Initialize(__TIME__, __FILE__, __LINE__, __FUNCSIG__, level))
#define LOG_INFO (LOG(INFO)) 
#define LOG_DEBUG (LOG(DEBUG)) 
#define LOG_WARNING (LOG(WARNING)) 
#define LOG_ERROR (LOG(ERROR)) 
#define LOG_FATAL (LOG(FATAL)) 