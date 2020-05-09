

#define LOCAL_H
#include "server.h"
#undef LOCAL_H

#include "Arduino.h"
#if defined(ESP8266)
#include <FS.h>
#elif defined(ESP32)
#include "SPIFFS.h"
#endif // ESP

#include "../Timers/timers.h"

#include "../clock/clock.h"

#if defined(UltraWebServer)
#include "..\RelayController\RelayController.h"
#include "../threceiver/threceiver.h"
#include "../Sensors/onewire_sensors.h"
#include "../config_store.h"

#elif defined(EnergyMonitor)

#include "../common_store.h"

#endif




#include "../esp_def.h"
//#include "pins_names.h"


#if defined(ESP8266)
extern "C" {
#include "user_interface.h"
}
#elif defined(ESP32)
extern "C" {
#include <esp_wifi.h>
}

#endif // ESP

#ifdef USE_FTP_SERVER
extern FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial
#endif


HTTPserverClass HTTPserver;

AsyncWebServerEx HTTPserverClass::server(80);
#ifdef USE_WEBSOCKETS
AsyncWebSocket HTTPserverClass::ws("/ws");
time_t HTTPserverClass::LastWebSocketActivityTimeLabel = 0;
#endif
AsyncStaticWebHandler* HTTPserverClass::serveStaticHandlerNA = nullptr;

String HTTPserverClass::help_info;

enum ContentTypes : uint8_t {
	application_octet_stream = 0, text_html, text_css, application_javascript, image_png,
	image_gif, image_jpeg, image_x_icon, text_xml, application_x_pdf,
	application_x_zip, application_x_gzip, text_plain, text_json
};

const char * ContentTypesStrings[] = { "application/octet-stream", "text/html", "text/css", "application/javascript", "image/png",
									   "image/gif", "image/jpeg", "image/x-icon", "text/xml", "application/x-pdf",
										"application/x-zip", "application/x-gzip", "text/plain", "text/json" };



#ifdef USE_FILES_READ

String HTTPserverClass::getContentType(String filename) {
	struct FileExtContentType {
		const char * ext;
		ContentTypes ContentType;
	};
	static FileExtContentType FileExtContentTypes[] = { {".htm" ,	ContentTypes::text_html },	{ ".html" , ContentTypes::text_html },
												{ ".css" ,	ContentTypes::text_css },	{ ".js" ,	ContentTypes::application_javascript },
												{ ".png" ,	ContentTypes::image_png },	{ ".gif" ,	ContentTypes::image_gif },
												{ ".jpg" ,	ContentTypes::image_jpeg },	{ ".ico" , ContentTypes::image_x_icon },
												{ ".xml" ,	ContentTypes::text_xml },	{ ".pdf" , ContentTypes::application_x_pdf },
												{ ".zip" ,	ContentTypes::application_x_zip },	{ ".gz" , ContentTypes::application_x_gzip },
												{ ".txt" ,	ContentTypes::text_plain },	{ ".btl" , ContentTypes::text_plain } };
//	if (server.arg("download"))
//		return ContentTypesStrings[ContentTypes::application_octet_stream];
	for (int i = 0; i < sizeofArray(FileExtContentTypes); i++) {
		if (filename.endsWith(FileExtContentTypes[i].ext))
			return ContentTypesStrings[FileExtContentTypes[i].ContentType];
	}
	return ContentTypesStrings[ContentTypes::text_plain];
}
#endif

#ifdef hasDemoMode
void return403_DemoMode(AsyncWebServerRequest * request) {
	return request->send(403, ContentTypesStrings[ContentTypes::text_plain], F("Demo mode"));
}
 
#endif // nondef

#ifdef Sonoff
void setSonoffRelayState(bool newState);
bool getSonoffRelayState();

void HTTPserverClass::handleSonoffRelay(AsyncWebServerRequest *request) {
	DEBUG_STACK();
	
	if (request->hasArg("state")) {
		if (request->arg("state") == "on") {
			setSonoffRelayState(true);
		}
		else if (request->arg("state") == "off") {
			setSonoffRelayState(false);
		}
		else if (request->arg("state") == "inv") {
			setSonoffRelayState(!getSonoffRelayState());
		}

		String output = getSonoffRelayState() ? "on" : "off";
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);

	}
	else {
		String output = "<head><title>Relay state</title></head><body>"
			"<form method=\"get\" action=\"relay\"><input type=\"hidden\" name=\"state\" value=\"on\"><button type=\"submit\">on</button></form>"
			"<form method=\"get\" action=\"relay\"><input type=\"hidden\" name=\"state\" value=\"off\" ><button type=\"submit\">off</button></form>"
			"<form method=\"get\" action=\"relay\"><input type=\"hidden\" name=\"state\" value=\"inv\" ><button type=\"submit\">inv</button></form>"
			"</body></html>";

		request->send(200, ContentTypesStrings[ContentTypes::text_html], output);
	}
}

#endif

#ifdef EnergyMonitor
void HTTPserverClass::handleEnergyMonitorSetup_GET(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char html[1024];
	sprintf_P(html, "<head><title>Config Energy monitor</title></head><body><form method=\"post\" action=\"em-setup\">"
		"Period T1 begin: <input name = \"PeriodT1begin\" length=\"4\" placeholder=\"hhmm\" value=\"%d\"><br>"
		"Period T2 begin: <input name = \"PeriodT2begin\" length=\"4\" placeholder=\"hhmm\" value=\"%d\"><br>"
		"Current energy T1: <input name = \"CurrentEnergyT1\" length=\"8\" placeholder=\"energy T1\" value=\"%ld\"><br>"
		"Current energy T2: <input name = \"CurrentEnergyT2\" length=\"8\" placeholder=\"energy T2\" value=\"%ld\"><br>"
		"<button type = \"submit\">save</button></form></body></html>",
		ConfigStore.Data.periodT1Begin, ConfigStore.Data.periodT2Begin,
		EnergyMonitorData.EnergyCountT1, EnergyMonitorData.EnergyCountT2);
	request->send(200, ContentTypesStrings[ContentTypes::text_html], html);
}

void HTTPserverClass::handleEnergyMonitorSetup_POST(AsyncWebServerRequest * request)
{
	if (request->hasArg(F("PeriodT1begin")) && request->hasArg(F("PeriodT2begin"))
		&& request->hasArg(F("CurrentEnergyT1")) && request->hasArg(F("CurrentEnergyT2"))) {

		void setInitDataPzem004tEnergyCount(uint16_t periodT1Begin, uint16_t periodT2Begin,
			uint32_t EnergyCountT1, uint32_t EnergyCountT2);
		setInitDataPzem004tEnergyCount(request->arg(F("PeriodT1begin")).toInt(),
			request->arg(F("PeriodT2begin")).toInt(),
			request->arg(F("CurrentEnergyT1")).toInt(),
			request->arg(F("CurrentEnergyT2")).toInt());
		handleEnergyMonitorInfo(request);
	}
	else
		request->send(400);
}

void HTTPserverClass::handleEnergyMonitorInfo(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char t1[32];
	sprintf(t1, "%s", time2str(ConfigStore.Data.EnergyCountTimeT1));
	char t2[32];
	sprintf(t2, "%s", time2str(ConfigStore.Data.EnergyCountTimeT2));
	char txt[1024];
	sprintf(txt, "Voltage:%d\nCurrent:%d\n"
		"Power:%d\n"
		"Energy:%ld\n"
		"Energy T1+T2:%ld\n"
		"Energy T1:%ld\nEnergy T2:%ld\n"
		"Period T1 begin:%d\nPeriod T2 begin:%d\n"
		"EnergyCountRaw:%ld\nEnergyCountDelta:%ld\n"
		"EnergyCountTimeT1:%s\nEnergyCountTimeT2:%s\n"
		"EnergyCountRawOverload:%ld\n"
		,
		EnergyMonitorData.voltage_x10, EnergyMonitorData.current_x100,
		EnergyMonitorData.power, 
		EnergyMonitorData.energy,
		EnergyMonitorData.EnergyCount,
		EnergyMonitorData.EnergyCountT1, EnergyMonitorData.EnergyCountT2,
		ConfigStore.Data.periodT1Begin, ConfigStore.Data.periodT2Begin,
		ConfigStore.Data.EnergyCountRaw, ConfigStore.Data.EnergyCountDelta,
		t1, t2,
		ConfigStore.Data.EnergyCountRawOverload
		);
	request->send(200, ContentTypesStrings[ContentTypes::text_plain], txt);
}


void HTTPserverClass::handleNetSetup_GET(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char html[1024];
	sprintf_P(html, "<head><title>Config network</title></head><body><form method=\"post\" action=\"net-setup\">"
		"Mqtt/udp server: <input name = \"Server\" length=\"15\" placeholder=\"xxx.xxx.xxx.xxx\" value=\"%s\"><br>"
		"Mqtt/udp port: <input name = \"Port\" length=\"6\" placeholder=\"port\" value=\"%d\"><br>"
		"<label>Narodmon: <input name = \"Narodmon\" type=\"checkbox\" %s></label><br>"
		"<button type = \"submit\">save</button></form></body></html>",
		ipaddr_ntoa(&ConfigStore.Data.MqttUdpServer), 
		ConfigStore.Data.MqttUdpPort,
		(ConfigStore.Data.SendDataToNarodmon ? "checked" : "")
	);
	request->send(200, ContentTypesStrings[ContentTypes::text_html], html);

}

void HTTPserverClass::handleNetSetup_POST(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	if (request->hasArg(F("Server")) && request->hasArg(F("Port"))
		//&& request->hasArg(F("Period"))
		) {

		ConfigStore.Data.MqttUdpServer.addr = ipaddr_addr(request->arg(F("Server")).c_str());
		ConfigStore.Data.MqttUdpPort = request->arg(F("Port")).toInt();
		ConfigStore.Data.SendDataToNarodmon = request->arg(F("Narodmon")) == String("on");
		ConfigStore.save();
		handleNetInfo(request);
	}
	else
		request->send(400);

}

void HTTPserverClass::handleNetInfo(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char txt[1024];
	sprintf_P(txt, "Mqtt/udp server:%s\nMqtt/udp port:%d\nNarodmon:%s\n",
		ipaddr_ntoa(&ConfigStore.Data.MqttUdpServer), ConfigStore.Data.MqttUdpPort,
		(ConfigStore.Data.SendDataToNarodmon ? "on" : "off")
	);
	request->send(200, ContentTypesStrings[ContentTypes::text_plain], txt);

}

/*
void HTTPserverClass::handleOneWireNet_GET(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char html[1024];
	sprintf_P(html, "<head><title>Config Energy monitor network</title></head><body><form method=\"post\" action=\"en-setup\">"
		"Mqtt/udp server: <input name = \"Server\" length=\"15\" placeholder=\"xxx.xxx.xxx.xxx\" value=\"%s\"><br>"
		"Mqtt/udp port: <input name = \"Port\" length=\"6\" placeholder=\"port\" value=\"%d\"><br>"
		//"Update period: <input name = \"Period\" length=\"3\" placeholder=\"period\" value=\"%d\"><br>"
		"<button type = \"submit\">save</button></form></body></html>",
		ipaddr_ntoa(&ConfigStore.Data.OneWireMqttUdpServer),
		ConfigStore.Data.OneWireMqttUdpPort);
	request->send(200, ContentTypesStrings[ContentTypes::text_html], html);
}

void HTTPserverClass::handleOneWireNet_POST(AsyncWebServerRequest * request)
{
	if (request->hasArg(F("Server")) && request->hasArg(F("Port"))
		//&& request->hasArg(F("Period"))
		) {

		ConfigStore.Data.OneWireMqttUdpServer.addr = ipaddr_addr(request->arg(F("Server")).c_str());
		ConfigStore.Data.OneWireMqttUdpPort = request->arg(F("Port")).toInt();
		//ConfigStore.Data.periodPublicDataPzem004 = request->arg(F("Period")).toInt();
		handleOneWireNet_Info(request);
	}
	else
		request->send(400);

}

void HTTPserverClass::handleOneWireNet_Info(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char txt[1024];
	sprintf_P(txt, "Mqtt/udp server:%s\nMqtt/udp port:%d\n"
		//"Update period:%d\n"
		, ipaddr_ntoa(&ConfigStore.Data.OneWireMqttUdpServer), ConfigStore.Data.OneWireMqttUdpPort
		//,ConfigStore.Data.periodPublicDataPzem004
	);
	request->send(200, ContentTypesStrings[ContentTypes::text_plain], txt);
}
*/
#endif


#ifdef USE_FILES_EDIT
void HTTPserverClass::handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
	DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif

	//DEBUG_STACK_PRINT(filename << " " << index << " " << len);
	if (!index) {
//		if (!_username.length() || request->authenticate(_username.c_str(), _password.c_str())) {
//			_authenticated = true;
			request->_tempFile = SPIFFS.open((filename.startsWith("/") ? "" : "/") + filename, "w");
//			_startTime = millis();
//		}
	}
//	if (_authenticated && request->_tempFile) {
	if (request->_tempFile) {
		if (len) {
			request->_tempFile.write(data, len);
			//DEBUG_PRINT(len);
		}
		if (final) {
			request->_tempFile.close();
			//DEBUG_PRINT(final);
		}
	}

/*
	//DEBUG_STACK();

	static File fsUploadFile;
	//HTTPUpload& upload = server.upload();
//	if (upload.status == UPLOAD_FILE_START) {
//		String filename = upload.filename;
		if (!filename.startsWith("/")) filename = "/" + filename;
		DEBUG_PRINT(F("FileName: ") << filename);
		fsUploadFile = SPIFFS.open(filename, "w");
//		filename = String();
	}
	else if (upload.status == UPLOAD_FILE_WRITE) {
		//Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
		if (fsUploadFile)
			fsUploadFile.write(upload.buf, upload.currentSize);
	}
	else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile)
			fsUploadFile.close();
		//DEBUG_PRINT(F("FileSize: ") << upload.totalSize);
	}
	*/
}

void HTTPserverClass::handleFileDelete(AsyncWebServerRequest *request) {
	DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	if (!request->hasParam("path", true))
		return request->send(400, ContentTypesStrings[ContentTypes::text_plain], "BAD PARAMS");
	String path = request->getParam("path", true)->value();
	//DEBUG_PRINT(path);
	if (path == "/")
		return request->send(400, ContentTypesStrings[ContentTypes::text_plain], "BAD PATH");
	if (!SPIFFS.exists(path))
		return request->send(404);//, ContentTypesStrings[ContentTypes::text_plain], "File not found");
	SPIFFS.remove(path);
	request->send(200, ContentTypesStrings[ContentTypes::text_plain], "");
}

void HTTPserverClass::handleFileCreate(AsyncWebServerRequest *request) {
	DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	if (!request->hasParam("path", true))
		return request->send(400, ContentTypesStrings[ContentTypes::text_plain], "BAD PARAMS");
	String path = request->getParam("path", true)->value();
	//DEBUG_PRINT(path);
	if (path == "/")
		return request->send(400, ContentTypesStrings[ContentTypes::text_plain], "BAD PATH");
	if (SPIFFS.exists(path))
		return request->send(400, ContentTypesStrings[ContentTypes::text_plain], "FILE EXISTS");
	File file = SPIFFS.open(path, "w");
	if (file)
		file.close();
	else
		return request->send(500, ContentTypesStrings[ContentTypes::text_plain], "CREATE FAILED");
	request->send(200, ContentTypesStrings[ContentTypes::text_plain], "");
}
#endif //  USE_EDIT_FILES

#ifdef  USE_FILES_READ

void HTTPserverClass::handleNotFound(AsyncWebServerRequest *request) {
	//DEBUG_STACK_PRINT(request->url());

	String path = "/404.htm";
	String gzip = path + ".gz";
	request->_tempFile = SPIFFS.open(path, "r");
	bool found = bool(request->_tempFile);
	if (!found) {
		request->_tempFile = SPIFFS.open(gzip, "r");
		found = bool(request->_tempFile);
	}
	if (!found) {
		String err404 = F("404: File not found\n");
		err404.concat(help_info);
		request->send(404, ContentTypesStrings[ContentTypes::text_plain], err404.c_str());
	}

//	DEBUG_PRINT("path:" << path << ",exist:" << exist << ", available:" << file.available());
/*	if (path.endsWith("/")) path += "index.htm";
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
		if (SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		size_t sent = server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;

	server.onNotFound(handleNotFound);
	[](AsyncWebServerRequest *request) {
	if (!handleFileRead(server.uri())) {
	if (!handleFileRead("/404.htm")) {
	String err404 = F("404: File not found\n");
	err404.concat(help_info);
	request->send(404, ContentTypesStrings[ContentTypes::text_plain], err404.c_str());
	}
	}
	});*/

}


void HTTPserverClass::handleFileList(AsyncWebServerRequest *request) {
	DEBUG_STACK();
	//panic();
	String path = request->arg("dir");

	DEBUG_PRINT(path);

#if defined(ESP8266)
	Dir dir = SPIFFS.openDir(path);
#elif defined(ESP32)
	File dir = SPIFFS.open(path);
#endif

	if (request->arg("format") == "json") {

		String output = "[";

#if defined(ESP8266)
		while (dir.next()) {
			File entry = dir.openFile("r");
#elif defined(ESP32)
		File entry = dir.openNextFile();
		while (entry) {
#endif

		if (output != "[") output += ',';
			output += "\n{\"type\":\"file\",\"name\":\"" + String(entry.name()) + "\",\"size\":" + String(entry.size()) + "}";

#if defined(ESP8266)
			entry.close();
#elif defined(ESP32)
			entry = dir.openNextFile();
#endif
		}
#ifdef ESP32
		dir.close();
#endif
		output += "\n]";
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);
	} else{
		String output;
#if defined(ESP8266)
		while (dir.next()) {
			File entry = dir.openFile("r");
#elif defined(ESP32)
		File entry = dir.openNextFile();
		while (entry) {
#endif
			output += String(entry.name()) + '\n';
#if defined(ESP8266)
			entry.close();
#elif defined(ESP32)
			entry = dir.openNextFile();
#endif
		}
#ifdef ESP32
		dir.close();
#endif
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}

}
#endif //  USE_FILES_READ

wl_status_t WiFi_waitForConnectResult(uint16_t timeout) {
	//1 and 3 have STA enabled
	if (!(WiFi.getMode() & WIFI_STA)) {
		return WL_DISCONNECTED;
	}
	ulong end = millis() + timeout;
	while (WiFi.status() == WL_DISCONNECTED && millis() < end) {
		delay(100);
		ESP_wdtFeed();
	}
	return WiFi.status();
}

wl_status_t WiFi_disconnect(uint16_t timeout) {
	//1 and 3 have STA enabled
	if (!(WiFi.getMode() & WIFI_STA)) {
		return WL_DISCONNECTED;
	}
	ulong end = millis() + timeout;
	while (WiFi.status() == WL_CONNECTED && millis() < end) {
		WiFi.disconnect();
		delay(100);
		ESP_wdtFeed();
	}
	return WL_DISCONNECTED;
}

void HTTPserverClass::handleSetWiFiConfig(AsyncWebServerRequest *request) {
	DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	if (request->hasArg(F("ssid")) && request->hasArg(F("password"))) {
		String result;
		String ssid = request->arg(F("ssid"));
		String password = request->arg(F("password"));
		DEBUG_PRINT(F("ssid:") << ssid << F(",password:") << password);
		if (ssid.length()) {
			// задано имя точки доступа. отключимся
			DISABLE_TIMER_INTERRUPTS();
			DEBUG_PRINT(F("Try WiFi.enableAP"));
			WiFi.enableAP(true); // включим режим станции и точки доступа
			WiFi_disconnect(3000);

			if (password.length()) {
				// задано имя точки доступа и пароль. будем подключаться
				DEBUG_PRINT(F("Try WiFi.begin"));

				WiFi.begin(ssid.c_str(), password.c_str()); // пробуем подключиться

				DEBUG_PRINT(F("WiFi.begin"));
				wl_status_t connRes = WiFi_waitForConnectResult(3000);
				if (connRes != wl_status_t::WL_CONNECTED) {
					result = F("Not connected:");
					switch (connRes) {
					case wl_status_t::WL_NO_SSID_AVAIL:
						result += F("WL_NO_SSID_AVAIL"); break;
					case wl_status_t::WL_CONNECT_FAILED:
						result += F("WL_CONNECT_FAILED"); break;
					case wl_status_t::WL_DISCONNECTED:
						result += F("WL_DISCONNECTED"); break;
					default:
						result += connRes; break;
					};
				}
				else
					result = F("Connected");
			}
			else
				result = F("Disconected");
			// если подключимся - сработает обработчик события, и точка доступа отключится
			ENABLE_TIMER_INTERRUPTS();
		}
		else {
			result = F("SSID or password not set");
		}
		DEBUG_PRINT(result);
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], result);
	}
	else
		request->send(400);
}


void HTTPserverClass::handleWiFiScan(AsyncWebServerRequest *request) {
	DEBUG_STACK();

	int8_t countNetworks = WiFi.scanComplete();
	DEBUG_PRINT(countNetworks);
	if (countNetworks == -2) {
		Timers.once([]() {/*ETS_FRC1_INTR_DISABLE();*/ WiFi.scanNetworks(true); /*ETS_FRC1_INTR_ENABLE();*/ }, 1);
		request->send(202, ContentTypesStrings[ContentTypes::text_plain], "202. Start scan..."); // no content
		return;
	}
	else if (countNetworks == -1) {
		request->send(202, ContentTypesStrings[ContentTypes::text_plain], "202. Scanning..."); // no content
		return;
	}

	//DEBUG_PRINT(WiFi.scanNetworks());

	if (request->arg("format") == "json") {

		String output = "[";
		//		ETS_FRC1_INTR_DISABLE();
		for (int i = 0; i < countNetworks; i++) {
			if (output != "[") output += ',';
			output += "\n{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + WiFi.RSSI(i) + ",\"enc\":" + WiFi.encryptionType(i) + "}";
		}
		output += "\n]";
		//		ETS_FRC1_INTR_ENABLE();
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}
	else {
		String output;
		//		ETS_FRC1_INTR_DISABLE();
		//DEBUG_PRINT(n);
		for (int i = 0; i < countNetworks; i++)
			output += WiFi.SSID(i) + "\t" + WiFi.RSSI(i) + "\t" + WiFi.encryptionType(i) + "\n";
		//		ETS_FRC1_INTR_ENABLE();
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}
	WiFi.scanDelete();

}


void HTTPserverClass::handleWiFiInfo(AsyncWebServerRequest *request) {
	DEBUG_STACK();


#if defined(ESP8266)
	softap_config config;

	wifi_softap_get_config(&config);

#elif defined(ESP32)
	wifi_config_t config_ap;
	esp_wifi_get_config(WIFI_IF_AP, &config_ap);
	auto config = config_ap.ap;

#endif

	char apname[sizeof(config.ssid) + 1];

	memset(apname, 0, sizeof(apname));
	strncpy(apname, (const char *)config.ssid, config.ssid_len ? sizeof(apname) : config.ssid_len);

	if (request->arg("format") == "json") {
		String output = F("{\n");
		output += String(F("\"mode\":")) + WiFi.getMode() + ",\n";
		output += String(F("\"ssid\":\"")) + WiFi.SSID() + "\",\n";
		output += String(F("\"ap\":\"")) + apname + "\"\n";
		output += String(F("\"MAC\":\"")) + WiFi.macAddress() + "\"\n";
		output += String(F("}"));
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}else{
		String output;
		output += String(F("mode:")) + WiFi.getMode() + "\n";
		output += String(F("ssid:")) + WiFi.SSID() + "\n";
		output += String(F("ap:")) + apname + "\n";
		output += String(F("MAC:")) + WiFi.macAddress() + "\n";

		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}
}


void HTTPserverClass::handleSetAPConfig(AsyncWebServerRequest *request) {
	DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	if (request->hasArg(F("apid")) && request->hasArg(F("password"))) {
		String result;
		String ssid = request->arg(F("apid"));
		String password = request->arg(F("password"));
		DEBUG_PRINT(F("apid:") << ssid << F(",password:") << password);
		if (ssid.length()) {
			DEBUG_PRINT(F("Try WiFi.enableAP"));
			// задано имя точки доступа. включим
			DISABLE_TIMER_INTERRUPTS();
			WiFi.softAP(ssid.c_str(), password.c_str());
			ENABLE_TIMER_INTERRUPTS();
			result = F("AP enabled");
		}
		else {
			result = F("APID not set");
		}
		DEBUG_PRINT(result);
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], result);
	}
	else
		request->send(400);
}

#ifdef useUserConfig
void HTTPserverClass::handleSetUserConfig(AsyncWebServerRequest *request) {
	DEBUG_STACK();

#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	if (request->hasArg(F("name")) && request->hasArg(F("password"))) {
		String result;
		String name = request->arg(F("name"));
		String password = request->arg(F("password"));
		DEBUG_PRINT(F("name:") << name << F(",password:") << password);

		ConfigStore.setAdminName(name.c_str());
		ConfigStore.setAdminPassword(password.c_str());
		setAuthentication(ConfigStore.getAdminName(), ConfigStore.getAdminPassword());
		result = F("Authentication set");
		
		DEBUG_PRINT(result);
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], result);
	}
	else
		request->send(400);
}
#endif // useUserConfig

#ifdef useUserConfig
void HTTPserverClass::handleUserInfo(AsyncWebServerRequest *request) {
	DEBUG_STACK();

	String output;

	if (request->arg("format") == "json") {
		String output = F("{\n");
		output += String(F("\"name\":\"")) + ConfigStore.getAdminName() + "\"\n";
		output += String(F("}"));
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}
	else {
		String output;
		output += String(F("name:")) + ConfigStore.getAdminName() + '\n';

		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}

}
#endif // useUserConfig

#ifdef USE_WEBSOCKETS

void HTTPserverClass::onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
	LastWebSocketActivityTimeLabel = Clock.uptime();

	if (type == AwsEventType::WS_EVT_CONNECT) {
		DEBUG_PRINT("ws:" << server->url() << ", id: " << client->id() << " connect" << endl);
#ifdef UltraWebServer
		ws.textAll("Hello!\nType #help for command list\n");
#endif
		client->ping();
	}else if (type == AwsEventType::WS_EVT_DISCONNECT) {
		//DEBUG_PRINT("ws:" << server->url() << ",id: " << client->id() << " disconnect" << endl);
	}else if (type == AwsEventType::WS_EVT_ERROR) {
		//DEBUG_PRINT("ws:" << server->url() << ",id: " << client->id() << " error:(" << *((uint16_t*)arg) << ")" << (char*)data <<  endl);
	}else if (type == AwsEventType::WS_EVT_PONG) {
		//DEBUG_PRINT("ws:" << server->url() << ",id: " << client->id() << " pong:(" << len << ")" << ((len) ? (char*)data : "") << endl);
	}else if (type == AwsEventType::WS_EVT_DATA) {
		AwsFrameInfo * info = (AwsFrameInfo*)arg;
		String msg = "";
		if (info->final && info->index == 0 && info->len == len) {
			//the whole message is in a single frame and we got all of it's data
			if (info->opcode == WS_TEXT) {
				for (size_t i = 0; i < info->len; i++) {
					msg += (char)data[i];
				}
			}
#ifdef UltraWebServer
			RelayController.parseCmdBuf((char*)msg.c_str());
#endif
		}
		else {
			//message is comprised of multiple frames or the frame is split into multiple packets
			if (info->opcode == WS_TEXT || info->opcode == WS_BINARY) {
				for (size_t i = 0; i < info->len; i++) {
					msg += (char)data[i];
				}
			}
		}
	}
}

void HTTPserverClass::wsSendText(String & text)
{
//	DEBUG_STACK_PRINT(text);
#ifdef UltraWebServer
	static unsigned long lastTime = 0;
	unsigned long currTime = millis();
	if ((currTime - lastTime) > 2000)
		DEBUG_PRINT(F("No send text more than two seconds"));
	lastTime = currTime;
#endif

	ws.textAll(text.c_str());
}
#endif // USE_WEBSOCKETS

#ifdef USE_NTP_CLIENT

#ifdef EnergyMonitor
void HTTPserverClass::handleSetNTPConfig(AsyncWebServerRequest *request) {
	if (request->hasArg(F("ServerName")) && request->hasArg(F("TimeZone"))) {
		String ServerName = request->arg(F("ServerName"));
		int TimeZone = request->arg(F("TimeZone")).toInt();
		if (ServerName.length()) {
			// задано имя сервера. сохраним
			ConfigStore.setTimeServer(ServerName.c_str());
			ConfigStore.setTimeZone(TimeZone);
			String res = "NTP server: " + ServerName + "\nTime zone: " + TimeZone;
			request->send(200, ContentTypesStrings[ContentTypes::text_plain], res);
		}else {
			request->send(200, ContentTypesStrings[ContentTypes::text_plain], F("NTP server name not set"));
		}

	}else
		request->send(400);
}


#else

void HTTPserverClass::handleSetNTPConfig(AsyncWebServerRequest *request) {
	//DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif

#ifdef EnergyMonitor
	if (request->hasArg(F("TimeZone"))) {
		int TimeZone = request->arg(F("TimeZone")).toInt();
		Clock.setTimeZone((ClockNtpClass::TimeZones)TimeZone);
		String res = "Time zone: " + Clock.getTimeZone();
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], res);
	//}else {
	//		request->send(200, ContentTypesStrings[ContentTypes::text_plain], F("Time zone not set"));
	}else
		request->send(400);
#else
	if (request->hasArg(F("ServerName")) && request->hasArg(F("TimeZone"))) {
		String ServerName = request->arg(F("ServerName"));
		int TimeZone = request->arg(F("TimeZone")).toInt();
		if (ServerName.length()) {
			// задано имя сервера. сохраним
			Clock.setTimeServer(ServerName);
			Clock.setTimeZone((ClockNtpClass::TimeZones)TimeZone);
			String res = "NTP server: " + ServerName + "\nTime zone: " + TimeZone;
			request->send(200, ContentTypesStrings[ContentTypes::text_plain], res);
		}
		else {
			request->send(200, ContentTypesStrings[ContentTypes::text_plain], F("NTP server name not set"));
		}
	
	}else
		request->send(400);
#endif

}
#endif
#endif // USE_NTP_CLIENT


#ifdef USE_RTC_CLOCK

void HTTPserverClass::handleSetRTC_time(AsyncWebServerRequest *request) {
	//DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	time_t nt;
	if (request->hasArg(F("newtime")) && (nt = str2time(request->arg(F("newtime")).c_str()))){
		Clock.setTime(nt);
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], time2str(Clock.now(), "%Y-%m-%dT%H:%M:%S"));
	}
	else
		request->send(400);
}
#endif // USE_RTC_CLOCK

void HTTPserverClass::handleUpdateFlash(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
	//DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
#if defined(ESP8266)

	if (!index) {
		DEBUG_PRINT(F("Update Start:") << filename.c_str());
		Update.runAsync(true);
		if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
			Update.printError(Serial);
		}
	}
	DEBUG_PRINT(F("Update part:") << index);
	if (!Update.hasError()) {
		if (Update.write(data, len) != len) {
			Update.printError(Serial);
			request->send(400);
		}
	}
	if (final) {
		if (Update.end(true)) {
			//ESP.restart();
			DEBUG_PRINT(F("Update Success:") << (index + len));
			request->send(200, ContentTypesStrings[ContentTypes::text_plain], "Ok. Wait for restart");
			Timers.once([]() {ESP.restart(); }, 1000);
		}
		else {
			Update.printError(Serial);
			request->send(400);
		}
	}


#elif defined(ESP32)

	DEBUG_PRINT(F("ApplyRelayState not imlemented"));

#endif

	
}


void HTTPserverClass::handleInfo(AsyncWebServerRequest *request) {
	DEBUG_STACK();

	String output;
	time_t uptime = Clock.uptime();

	if (request->arg("format") == "json") {
		String output = F("{\n");
#ifdef hasDemoMode
		if (ConfigStore.getDemoMode())
			output += String(F("\"DemoMode\":true\n,"));
#endif // ConfigStore
#if defined(ESP8266)
		output += String(F("\"MCU\":\"ESP8266\"\n"));
#elif defined(ESP32)
		output += String(F("\"MCU\":\"ESP32\"\n"));
#else
		output += String(F("\"MCU\":\"unknown\"\n"));
#endif
		output += String(F("\"ChipID\":\"")) + ESP_getChipIdStr() + "\"\n";
		output += String(F("\"FreeHeap\":\"")) + String(ESP.getFreeHeap()) + "\"\n";
		output += String(F("\"Version\":\"")) + String(__DATE__) + "\"\n";
		output += String(F("\"Uptime\":\"")) + String((int)(uptime / 86400ul)) + time2str(uptime, "D%H%M%S") + "\"\n";
		output += String(F("}"));
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}
	else {
		String output;
#if defined(ESP8266)
		output += String(F("MCU:ESP8266\n"));
#elif defined(ESP32)
		output += String(F("MCU:ESP32\n"));
#else
		output += String(F("MCU:unknown\n"));
#endif
		output += String(F("ChipID:")) + ESP_getChipIdStr() + '\n';
		output += String(F("FreeHeap:")) + String(ESP.getFreeHeap()) + '\n';
		output += String(F("Version:")) + String(__TIMESTAMP__) + '\n';
		output += String(F("Uptime:")) + String((int)(uptime / 86400ul)) + time2str(uptime, "D%H%M%S") + '\n';

#if defined(hasDemoMode)
		if (ConfigStore.getDemoMode())
			output += String(F("DemoMode:true\n"));
#endif // ConfigStore

		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}

}

/*void HTTPserverClass::handleUptime(AsyncWebServerRequest *request) {
	DEBUG_STACK();

	time_t tt = Clock.uptime();
	DEBUG_PRINT(F("uptime:") << tt);
	String output = String((int)(tt / 86400ul)) + time2str(Clock.uptime(), "D%H%M%S");

	request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);

}*/

void HTTPserverClass::init()
{
	//SERVER INIT
	help_info.concat(F("/'doc_hame.ext': load file from server. Allow methods: HTTP_GET\n"));
	AsyncStaticWebHandler& handler = server.serveStatic("na/", SPIFFS, "na/");
	serveStaticHandlerNA = &handler;
	server.serveStatic("/", SPIFFS, "/");

#ifdef Sonoff 
	//info
	help_info.concat(F("/relay ? [state=on|off|inv]: get or set relay state. Allow methods: HTTP_GET\n"));
	server.on("/relay", HTTP_GET, handleSonoffRelay);
#endif
	
#ifdef EnergyMonitor
	help_info.concat(F("/em-setup: set Energy monitor config. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlEM_setup = "/em-setup";
	server.on(urlEM_setup, HTTP_GET, handleEnergyMonitorSetup_GET);
	server.on(urlEM_setup, HTTP_POST, handleEnergyMonitorSetup_POST);

	help_info.concat(F("/em-info: get Energy monitor info. Allow methods: HTTP_GET\n"));
	server.on("/em-info", HTTP_GET, handleEnergyMonitorInfo);

	help_info.concat(F("/net-info: set Network info. Allow methods: HTTP_GET\n"));
	server.on("/net-info", HTTP_GET, handleNetInfo);

	help_info.concat(F("/net-setup: set Network setup. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlEM_net = "/net-setup";
	server.on(urlEM_net, HTTP_GET, handleNetSetup_GET);
	server.on(urlEM_net, HTTP_POST, handleNetSetup_POST);

#endif
	//info
	help_info.concat(F("/info: get system info. Allow methods: HTTP_GET\n"));
	server.on("/info", HTTP_GET, handleInfo);
#if defined(USE_RTC_CLOCK) || defined(USE_NTP_CLIENT)
	help_info.concat(F("/time: get time as string (eg: 20140527T123456). Allow methods: HTTP_GET\n"));
	server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/time")); request->send(200, ContentTypesStrings[ContentTypes::text_plain], time2str(Clock.now(), "%Y%m%dT%H%M%S")); });
#endif	
//	help_info.concat(F("/uptime: get uptime as string (eg: 123D123456). Allow methods: HTTP_GET\n"));
//	server.on("/uptime", HTTP_GET, handleUptime);

#ifdef USE_NTP_CLIENT
	help_info.concat(F("/ntp: set NTP config. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlNTP = "/ntp";
#ifdef EnergyMonitor
	server.on(urlNTP, HTTP_GET, [](AsyncWebServerRequest *request) {
		DEBUG_PRINT(F("/ntp"));
		char html[512];
		sprintf_P(html, "<head><title>Config NTP</title></head><body><form method=\"post\" action=\"ntp\"><input name=\"ServerName\" placeholder=\"Server name\" value=\"%s\"><input name=\"TimeZone\" length=\"3\" placeholder=\"Time zone\" value=\"%d\"><button type=\"submit\">save</button></form></body></html>",
			Clock.getTimeServer(), Clock.getTimeZone());
		request->send(200, ContentTypesStrings[ContentTypes::text_html], html);
	});
	
#else
	server.on(urlNTP, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/ntp")); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Config NTP</title></head><body><form method=\"post\" action=\"ntp\"><input name=\"ServerName\" length=\"64\" placeholder=\"Server name\"><input name=\"TimeZone\" length=\"3\" placeholder=\"Time zone\"><button type=\"submit\">save</button></form></body></html>"))); });
#endif
	server.on(urlNTP, HTTP_POST, handleSetNTPConfig);

#endif // USE_NTP_CLIENT

#ifdef USE_RTC_CLOCK
	help_info.concat(F("/rtc: set RTC time. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlNTP = "/rtc";
	server.on(urlNTP, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/rtc")); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>RTC time</title></head><body><form method=\"post\" action=\"rtc\"><input name=\"newtime\" length=\"15\" placeholder=\"yyyyMMddThhmmss\"><button type=\"submit\">set</button></form></body></html>"))); });
	server.on(urlNTP, HTTP_POST, handleSetRTC_time);
#endif // USE_RTC_CLOCK

#ifdef  USE_FILES_READ
	//list directory
	help_info.concat(F("/list ? [dir=...] & [format=json]: get file list as text or json. Allow methods: HTTP_GET\n"));
	server.on("/list", HTTP_GET, handleFileList);
#endif // USE_FILES_READ

#ifdef  USE_FILES_EDIT
	//simple file uploader 
	help_info.concat(F("/edit: edit files. Allow methods: HTTP_GET, HTTP_PUT, HTTP_DELETE, HTTP_POST\n"));
	const char * urlEdit = "/edit";
	server.on(urlEdit, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/edit:GET")); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>File upload</title></head><body><form action=\"/edit\" method=\"POST\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"><input type=\"submit\" value=\"Upload\"></form></body></html>"))); });
	//create file
	server.on(urlEdit, HTTP_PUT, handleFileCreate);
	//delete file
	server.on(urlEdit, HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on(urlEdit, HTTP_POST, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/edit:POST")); request->send(200, ContentTypesStrings[ContentTypes::text_plain], String(F("File uploaded"))); }, handleFileUpload);
#endif // USE_FILES_EDIT

	//simple WiFi config 
	help_info.concat(F("/wifi: edit wifi settings. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlWiFi = "/wifi";
	server.on(urlWiFi, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/wifi"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Config WiFi</title></head><body><form method=\"post\" action=\"wifi\"><input name=\"ssid\" length=\"32\" placeholder=\"ssid\"><input name=\"password\" length=\"64\" type=\"password\" placeholder=\"password\"><button type=\"submit\">save</button></form></body></html>"))); });
	server.on(urlWiFi, HTTP_POST, handleSetWiFiConfig);

	//list wifi networks
	help_info.concat(F("/wifi-scan ? [format=json]: get wifi list as text or json. Allow methods: HTTP_GET\n"));
	server.on("/wifi-scan", HTTP_GET, handleWiFiScan);

	help_info.concat(F("/wifi-info ? [format=json]: get wifi info as text or json. Allow methods: HTTP_GET\n"));
	server.on("/wifi-info", HTTP_GET, handleWiFiInfo);

#ifdef _ConfigStore
	//simple ap config 
	help_info.concat(F("/ap: edit soft ap settings. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlAP = "/ap";
	server.on(urlAP, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/ap"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Config AP</title></head><body><form method=\"post\" action=\"ap\"><input name=\"apid\" length=\"32\" placeholder=\"apid\"><input name=\"password\" length=\"64\" type=\"password\" placeholder=\"password\"><button type=\"submit\">save</button></form></body></html>"))); });
	server.on(urlAP, HTTP_POST, handleSetAPConfig);

	//simple user config 
	help_info.concat(F("/user: edit user settings. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlUser = "/user";
	server.on(urlUser, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/user"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Config User</title></head><body><form method=\"post\" action=\"user\"><input name=\"name\" length=\"15\" placeholder=\"name\"><input name=\"password\" length=\"15\" type=\"password\" placeholder=\"password\"><button type=\"submit\">save</button></form></body></html>"))); });
	server.on(urlUser, HTTP_POST, handleSetUserConfig);
	help_info.concat(F("/user-info ? [format=json]: get user info as text or json. Allow methods: HTTP_GET\n"));
	server.on("/user-info", HTTP_GET, handleUserInfo);
#endif // ConfigStore

	// update flash 
	help_info.concat(F("/update: update flash. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlUpdate = "/update";
	server.on(urlUpdate, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/update"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Update</title></head><body><form action=\"/update\" method=\"POST\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"><input type=\"submit\" value=\"update\"></form></body></html>"))); });
	server.on(urlUpdate, HTTP_POST, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/update:POST")); request->send(200, ContentTypesStrings[ContentTypes::text_plain], String(F("File uploaded")));}, handleUpdateFlash);

	// update flash 
	help_info.concat(F("/restart: restart system. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlRestart = "/restart";
	server.on(urlRestart, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/restart"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Restart</title></head><body><form action=\"/restart\" method=\"POST\" \"><input type=\"submit\" value=\"restart\"></form></body></html>"))); });
	server.on(urlRestart, HTTP_POST, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/restart:POST")); request->send(200, ContentTypesStrings[ContentTypes::text_plain], "Ok. Wait for restart"); Timers.once([]() {ESP.restart(); }, 1000);});

#ifdef USE_WEBSOCKETS
	help_info.concat(F("/ws: web socket url. Allow methods: HTTP_GET\n"));
	//	server.on("/ws", HTTP_GET, handleWebSocket);
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
#endif

#ifdef  USE_FILES_READ
	//called when the url is not defined here
	//use it to load content from SPIFFS
	server.onNotFound(handleNotFound);
	/*[](AsyncWebServerRequest *request) {
		if (!handleFileRead(server.uri())) {
			if (!handleFileRead("/404.htm")) {
				String err404 = F("404: File not found\n");
				err404.concat(help_info);
				request->send(404, ContentTypesStrings[ContentTypes::text_plain], err404.c_str());
			}
		}
	});*/
#else
	server.onNotFound([](AsyncWebServerRequest *request) {
		String err404 = F("404: File not found\n");
		err404.concat(help_info);
		request->send(404, ContentTypesStrings[ContentTypes::text_plain], err404.c_str());
	});
#endif // USE_FILES_READ

	//help_info.concat("</body></html>");
	help_info.concat(F("/help: list allow URLs. Allow methods: HTTP_GET\n"));
	server.on("/help", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, ContentTypesStrings[ContentTypes::text_plain], help_info.c_str()); });


#ifdef _ConfigStore
	setAuthentication(ConfigStore.getAdminName(), ConfigStore.getAdminPassword());

#endif // ConfigStore
//	ws.setAuthentication("admin", ConfigStore.getAdminPassword());
	
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

//	server.setAuthentication("user", "pass");
	server.begin();
//	Timers.add([]() {server.handleClient(); }, 10, F("server.handleClient()"));

#ifdef USE_DNS
//	MDNS.begin(host);
	// Add service to MDNS
//	MDNS.addService("http", "tcp", 80);

#endif // USE_DNS

	DEBUG_PRINT(F("HTTP server started"));

}
