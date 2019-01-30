#include <cassert>
#include "SimpleLogger.h"

SimpleLogger SimpleLogger::DefaultLogger = SimpleLogger();

std::ostream& operator<<(std::ostream& out, const LogLevel value)
{
	switch (value)
	{
	case info:
		out << "INFO";
		break;
	case debug:
		out << "DEBUG";
		break;
	case warning:
		out << "WARNING";
		break;
	case error:
		out << "ERROR";
		break;
	case fatal:
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

const LogStream& LogStream::Initialize(const char* time, const char* file, const int line, const char* funcsig, LogLevel level)
{
	assert(!initialized);
	initialized = level >= this->level;
	if (!initialized) return *this;

	currentTime = time;
	currentFile = file;
	currentLine = line;
	currentFuncSig = funcsig;
	currentLevel = level;

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

const LogStream& LogStream::Finalize(manipulator pf)
{
	if (!initialized) return *this;
	bool escaping = false;
	for (size_t i = fmtPtr + 1; i < format.size(); ++i)
	{

		if (escaping)
		{
			escaping = false;
			switch (format[i])
			{
			case '$':
				ostream << '$';
				break;
			case 't':
				ostream << currentTime;
				break;
			case 'f':
				ostream << currentFile;
				break;
			case 'l':
				ostream << currentLine;
				break;
			case 's':
				ostream << currentFuncSig;
				break;
			case 'v':
				ostream << currentLevel;
				break;
			default:
				assert(true);
				break;
			}
		}
		else if (format[i] == '$')
		{
			escaping = true;
		}
		else
		{
			ostream << format[i];
		}
	}
	ostream << pf;
	initialized = false;
	return *this;
}

void SimpleLogger::Add(const LogStream& logStream)
{
	logStreams.push_back(logStream);
}

SimpleLogger& SimpleLogger::Initialize(const char* time, const char* file, const int line, const char* funcsig, LogLevel level)
{
	for (auto& ls : logStreams)
	{
		ls.Initialize(time, file, line, funcsig, level);
	}
	return *this;
}

SimpleLogger& SimpleLogger::operator<<(manipulator pf)
{
	for (auto& logStream : logStreams)
	{
		logStream.Finalize(pf);
	}

	return *this;
}