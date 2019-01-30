#include <cassert>
#include "SimpleLogger.h"

static SimpleLogger DefaultLogger = SimpleLogger();

std::ostream& operator<<(std::ostream& out, const LogLevel value)
{
	switch (value)
	{
	case INFO:
		out << "INFO";
		break;
	case DEBUG:
		out << "DEBUG";
		break;
	case WARNING:
		out << "WARNING";
		break;
	case ERROR:
		out << "ERROR";
		break;
	case FATAL:
		out << "FATAL";
		break;
	}

	return out;
}

LogStream::LogStream(const LogLevel logLevel, std::ostream& os, const std::string& fmt) : ostream(os)
{
	level = logLevel;
	format = fmt;
	initialized = false;
}

const LogStream& LogStream::Initialize(const char* time, const char* file, const char* line, const char* funcsig, LogLevel level)
{
	assert(initialized);
	initialized = true;
	bool escaping = false;
	for (fmtPtr = 0; fmtPtr < format.size(); ++fmtPtr)
	{
		if (escaping)
		{
			escaping = false;
			switch (format[fmtPtr])
			{
			case '$':
				ostream << '$';
				break;
			case 't':
				ostream << time;
				break;
			case 'f':
				ostream << file;
				break;
			case 'l':
				ostream << line;
				break;
			case 's':
				ostream << funcsig;
				break;
			case 'v':
				ostream << level;
				break;
			case 'm':
				return *this;
			default:
				assert(true);
				break;
			}
		}
		else if (format[fmtPtr] == '$')
		{
			escaping = true;
		}
		else
		{
			ostream << format[fmtPtr];
		}

	}
	return *this;
}

void SimpleLogger::Add(LogStream& logStream)
{
	logStreams.push_back(logStream);
}

const SimpleLogger& SimpleLogger::Initialize(const char* time, const char* file, const char* line, const char* funcsig, LogLevel level)
{
	for each (LogStream ls in logStreams)
	{
		if (level >= ls.level)
		{
			ls.Initialize(time, file, line, funcsig, level);
		}
	}
	return *this;
}

template<typename T>
const SimpleLogger& SimpleLogger::operator<<(const T& buf) const
{
	for each (LogStream ls in logStreams)
	{
		ls << buf;
	}
	return *this;
}


const SimpleLogger& SimpleLogger::operator<<(LogStream::manipulator pf) const
{
	for each (LogStream ls in logStreams)
	{
		ls << pf;
	}
	return *this;
}

template <typename T>
const LogStream& LogStream::operator<<(const T& buf) const
{
	if (initialized)
		ostream << buf;
	return *this;
}

const LogStream& LogStream::operator<<(manipulator pf)
{
	if (initialized)
	{
		ostream << pf;
		initialized = false;
	}
	return *this;
}

