#pragma once
#include <ostream>
#include <string>
#include <vector>

enum LogLevel
{
	info = 0,
	debug = 1,
	warning = 2,
	error = 3,
	fatal = 4
};

class LogStream
{
	friend class SimpleLogger;
public:
	LogStream(LogLevel logLevel, std::ostream& os, const std::string& fmt = "[$v] $m");
	~LogStream() = default;

	typedef std::ostream& (*manipulator)(std::ostream&);
	template<typename T>
	const LogStream& operator<<(const T& buf) const;

	const LogStream& Initialize(const char* time, const char* file, const int line, const char* funcsig, LogLevel level);
	const LogStream& Finalize(manipulator pf);
private:
	LogLevel level;
	std::ostream& ostream;

	std::string format;

	bool initialized;

	std::string currentTime;
	std::string currentFile;
	int currentLine;
	std::string currentFuncSig;
	LogLevel currentLevel;

	size_t fmtPtr;
};

class SimpleLogger
{
public:
	SimpleLogger() = default;
	~SimpleLogger() = default;

	void Add(const LogStream& logStream);
	SimpleLogger& Initialize(const char* time, const char* file, int line, const char* funcsig, LogLevel level);

	typedef std::ostream& (*manipulator)(std::ostream&);
	template<class T>
	SimpleLogger& operator<<(const T& buf);

	SimpleLogger& operator<<(manipulator pf);

	static SimpleLogger DefaultLogger;
private:
	std::vector<LogStream> logStreams;
};

template <typename T>
const LogStream& LogStream::operator<<(const T& buf) const
{
	if (initialized)
		ostream << buf;
	return *this;
}

template<typename T>
SimpleLogger& SimpleLogger::operator<<(const T& buf)
{
	for (size_t i = 0; i < logStreams.size(); ++i)
	{
		logStreams[i] << buf;
	}
	return *this;
}

#define ADD_LOGGER(level, os) (SimpleLogger::DefaultLogger.Add(LogStream(level, os)))
#define ADD_LOGGER_FMT(level, os, fmt) (SimpleLogger::DefaultLogger.Add(LogStream(level, os, fmt)))
#define LOG(level) (SimpleLogger::DefaultLogger.Initialize(__TIME__, __FILE__, __LINE__, __FUNCSIG__, level))
#define LOG_INFO (LOG(info)) 
#define LOG_DEBUG (LOG(debug)) 
#define LOG_WARNING (LOG(warning)) 
#define LOG_ERROR (LOG(error)) 
#define LOG_FATAL (LOG(fatal)) 