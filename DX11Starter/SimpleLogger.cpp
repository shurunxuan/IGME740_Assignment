#include "SimpleLogger.h"

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
}

const LogStream& LogStream::Initialize(const char* file, const char* line, const char* funcsig, LogLevel level)
{
	bool escaping = false;
	for (size_t i = 0; i < format.size(); ++i)
	{
		switch (format[i])
		{
		case '$':
		{
			if (escaping)
			{
				ostream << '$';
				escaping = false;
			}
			else escaping = true;
		}
		break;
		case 'f':
		{
			if (escaping)
			{
				ostream << file;
				escaping = false;
			}
			else ostream << 'f';
		}
		break;
		case 'l':
		{
			if (escaping)
			{
				ostream << line;
				escaping = false;
			}
			else ostream << 'l';
		}
		break;
		case 's':
		{
			if (escaping)
			{
				ostream << funcsig;
				escaping = false;
			}
			else ostream << 's';
		}
		break;
		case 'v':
		{
			if (escaping)
			{
				ostream << level;
				escaping = false;
			}
			else ostream << 'v';
		}
		break;
		case 'm':
		{
			if (escaping)
			{
				return *this;
			}
			ostream << 'v';
		}
		break;
		default:
		{
			ostream << i;
		}
		break;
		}
	}
	return *this;
}

void SimpleLogger::Add(LogStream& logStream)
{
	logStreams.push_back(logStream);
}

const SimpleLogger& SimpleLogger::Initialize(const char* file, const char* line, const char* funcsig, LogLevel level)
{

	return *this;
}


template <typename T>
const LogStream& LogStream::operator<<(const T& buf) const
{
	ostream << buf;
	return *this;
}
