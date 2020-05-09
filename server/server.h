#ifndef SERVER_H
#define SERVER_H

#include <arduino.h>

#if defined(ESP8266)
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
//#include <AsyncTCP.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>



#ifdef LOCAL_H
#include "../common_def.h"
#else

#endif // LOCAL_H

#ifdef USE_FTP_SERVER
#include <ESP8266FtpServer.h>

extern FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial
#endif // USE_FTP_SERVER


class AsyncWebServerEx : public AsyncWebServer {
	// in ESPAsyncWebServer-master\src\WebHandlerImpl.h
	// need add:
	// ...handleRequest(...)
	// if ((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
	//	  return request->requestAuthentication();

public:
	AsyncWebServerEx(uint16_t port) : AsyncWebServer(port) {};

	void setAuthentication(const char *username, const char *password) {
		for (const auto& h : _handlers) {
			h->setAuthentication(username, password);
		}
	};
};

class HTTPserverClass {
public:
	static time_t LastWebSocketActivityTimeLabel;
private:
	static const char * AdminUserName;

	static String help_info;
	static AsyncWebServerEx server;

	static void handleInfo(AsyncWebServerRequest *request);
	//static void handleUptime(AsyncWebServerRequest *request);

#ifdef USE_FILES_READ
private:
	static String getContentType(String filename);
	static void handleNotFound(AsyncWebServerRequest *request);
	static void handleFileList(AsyncWebServerRequest *request);
#endif

#ifdef USE_FILES_EDIT
private:
	static void handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
	static void handleFileDelete(AsyncWebServerRequest *request);
	static void handleFileCreate(AsyncWebServerRequest *request);
#endif

#ifdef USE_NTP_CLIENT
private:
	static void handleSetNTPConfig(AsyncWebServerRequest *request);
#endif

#ifdef USE_RTC_CLOCK
private:
	static void handleSetRTC_time(AsyncWebServerRequest *request);
#endif

private:
	static void handleSetWiFiConfig(AsyncWebServerRequest *request);
	static void handleWiFiScan(AsyncWebServerRequest *request);
	static void handleWiFiInfo(AsyncWebServerRequest *request);
	static void handleSetAPConfig(AsyncWebServerRequest *request);
	static void handleSetUserConfig(AsyncWebServerRequest *request);
	static void handleUserInfo(AsyncWebServerRequest *request);

	static void handleUpdateFlash(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);

#ifdef Sonoff
	static void handleSonoffRelay(AsyncWebServerRequest *request);	
#endif // Sonoff

#ifdef EnergyMonitor 
	static void handleEnergyMonitorSetup_GET(AsyncWebServerRequest *request);
	static void handleEnergyMonitorSetup_POST(AsyncWebServerRequest *request);
	static void handleEnergyMonitorInfo(AsyncWebServerRequest *request);
	static void handleNetSetup_GET(AsyncWebServerRequest *request);
	static void handleNetSetup_POST(AsyncWebServerRequest *request);
	static void handleNetInfo(AsyncWebServerRequest *request);
/*	static void handleOneWireNet_GET(AsyncWebServerRequest *request);
	static void handleOneWireNet_POST(AsyncWebServerRequest *request);
	static void handleOneWireNet_Info(AsyncWebServerRequest *request);
*/
#endif

#ifdef USE_WEBSOCKETS
private:
	static void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
	static AsyncWebSocket ws;
public:
	static void wsSendText(String& text);
#endif // USE_WEBSOCKETS


private:
	static AsyncStaticWebHandler* serveStaticHandlerNA;
	static void setAuthentication(const char *username, const char *password) {
		server.setAuthentication(username, password);
		serveStaticHandlerNA->setAuthentication("", ""); // Файлы отдаем без авторизации - это надо для работы в режиме приложенния

#ifdef USE_WEBSOCKETS
#ifdef _ConfigStore

		ws.setAuthentication(username, password);
#endif
#endif // USE_WEBSOCKETS

#ifdef USE_FTP_SERVER
		ftpSrv.begin(username, password);
#endif // USE_FTP_SERVER
	}

public:
	static void init();


};

extern HTTPserverClass HTTPserver;

#endif //SERVER_H
