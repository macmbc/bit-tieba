#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	bool HandleGet(const std::string&, std::shared_ptr<HttpConnection>);
	void RegGet(const std::string&, HttpHandler handler);
	void RegPost(const std::string&, HttpHandler handler);
	bool HandlePost(const std::string&, std::shared_ptr<HttpConnection>);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};
