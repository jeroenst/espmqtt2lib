#ifndef ESPMQTT2LIB_h
#define ESPMQTT2LIB_h

using namespace std;

#include <EEPROM.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <RemoteDebug.h>


class ESPMQTT2LIB
{
	private:
		WiFiManager wifiManager;
		RemoteDebug Debug;
		ESP8266WebServer* webServer;

		unsigned int  wm_timeout   = 120; // seconds to run for
		unsigned int  wm_startTime = millis();
		bool wm_portalRunning      = false;
		bool wm_startAP            = false; // start AP and webserver if true, else start only webserver
		bool startWiFiManager      = false;
		bool wifiManagerBootStarted = false;
		bool wifiConnected 			= false;
		bool mqttConnecting			= false;
		bool mqttConnected			= false;
		int uptime					= 0;

		Ticker systemTimer;
		void wm_loop();
		static void wm_saveParamsCallback();
		void systemTimerCallback();
		//void handleWWWroot();
		//void handleNotFound();
		void webServerCallback();
		void handleWWWCustom();


	public:
		int wm_triggerpin = -1;
		ESPMQTT2LIB();
		void setup();
		void loop();

		void sethostname(const char* hostname);
		int setvalue(const char* topic, const char* value);
		const char* getvalue(const char* topic);
		int publishvalue(const char* topic);
		int publishvalue(const char* topic, const char* value);
		int publishvalues();
		void setSerialDebug(bool enable);
};

#endif