
#pragma once

#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/smart_ptr.hpp>

#include "halTorrent.hpp"
#include "global/string_conv.hpp"

namespace hal 
{

class Event
{
public:
	enum eventLevel { debug, info, warning, critical, fatal, none };
	
	enum codes {
		unclassified = HAL_EVENT_UNCLASSIFIED,
		debugEvent = HAL_EVENT_DEBUG,
		invalidTorrent = HAL_EVENT_INVTORRENT,
		torrentException = HAL_EVENT_TORRENTEXP,
		generalException = HAL_EVENT_EXP,
		xmlException = HAL_EVENT_XML_EXP,
		unicodeException = HAL_EVENT_UNICODE_EXP,
		peer = HAL_EVENT_PEER,
		tracker = HAL_EVENT_TRACKER
	};
	
	static std::wstring eventLevelToStr(eventLevel);	
	void post(boost::shared_ptr<EventDetail> event);
	
	boost::signals::scoped_connection attach(boost::function<void (shared_ptr<EventDetail>)> fn)
	{
		return event_signal_.connect(fn);
	}
	
private:
	boost::signal<void (shared_ptr<EventDetail>)> event_signal_;
};

Event& event();

class EventDetail
{
public:
	EventDetail(Event::eventLevel l, boost::posix_time::ptime t, Event::codes c) :
		level_(l),
		timeStamp_(t),
		code_(c)
	{}
	virtual ~EventDetail() 
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(L"Code %1%") % code()).str();
	}

	Event::eventLevel level() { return level_; }
	boost::posix_time::ptime timeStamp() { return timeStamp_; }
	Event::codes code() { return code_; }
	
private:	
	Event::eventLevel level_;
	boost::posix_time::ptime timeStamp_;
	Event::codes code_;
};

class EventLibtorrent : public EventDetail
{
public:
	EventLibtorrent(Event::eventLevel l, boost::posix_time::ptime t, Event::codes c, std::wstring m) :
		EventDetail(l, t, c),
		msg_(m)
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(code())) % msg_).str();
	}
	
private:
	std::wstring msg_;
};

class EventPeerAlert : public EventDetail
{
public:
	EventPeerAlert(Event::eventLevel l, boost::posix_time::ptime t, std::wstring m) :
		EventDetail(l, t, Event::peer),
		msg_(m)
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(code())) % msg_).str();
	}
	
private:
	std::wstring msg_;
};

class EventXmlException : public EventDetail
{
public:
	EventXmlException(std::wstring e, std::wstring m) :
		EventDetail(Event::warning, boost::posix_time::second_clock::universal_time(), Event::xmlException),
		exp_(e),
		msg_(m)
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(HAL_EVENT_XMLEXP)) % exp_ % msg_).str();
	}
	
private:
	std::wstring exp_;
	std::wstring msg_;
};

class EventInvalidTorrent : public EventDetail
{
public:
	EventInvalidTorrent(Event::eventLevel l, Event::codes code, std::string t, std::string f) :
		EventDetail(l, boost::posix_time::second_clock::universal_time(), code),
		torrent_(hal::from_utf8(t)),
		function_(hal::from_utf8(f))
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(code())) % torrent_).str();
	}
	
private:
	std::wstring function_;
	std::wstring torrent_;
	std::wstring exception_;
};

class EventTorrentException : public EventDetail
{
public:
	EventTorrentException(Event::eventLevel l, Event::codes code, std::string e, std::string t, std::string f) :
		EventDetail(l, boost::posix_time::second_clock::universal_time(), code),
		torrent_(hal::from_utf8(t)),
		function_(hal::from_utf8(f)),
		exception_(hal::from_utf8(e))
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(code())) % torrent_ % exception_ % function_).str();
	}
	
private:
	std::wstring torrent_;
	std::wstring function_;
	std::wstring exception_;
};

class EventStdException : public EventDetail
{
public:
	EventStdException(Event::eventLevel l, const std::exception& e, std::wstring from) :
		EventDetail(l, boost::posix_time::second_clock::universal_time(), Event::generalException),
		exception_(hal::from_utf8(e.what())),
		from_(from)
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(code())) % exception_ % from_).str();
	}
	
private:
	std::wstring exception_;
	std::wstring from_;
};

class EventDebug : public EventDetail
{
public:
	EventDebug(Event::eventLevel l, std::wstring msg) :
		EventDetail(l, boost::posix_time::second_clock::universal_time(), Event::debugEvent),
		msg_(msg)
	{}
	
	virtual std::wstring msg()
	{
		return (wformat(hal::app().res_wstr(code())) % msg_).str();
	}
	
private:
	std::wstring msg_;
};

class EventSession : public EventDetail
{

};

}// namespace hal