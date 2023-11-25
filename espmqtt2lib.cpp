#include "espmqtt2lib.h"
#include <LinkedList.h>

// Let's define a new class
class EspMqtt2Value {
	public:
		char *name;
		char *value;
};

LinkedList<EspMqtt2Value*> espMqtt2ValueList = LinkedList<EspMqtt2Value*>();

WiFiManagerParameter custom_mqtt_server = {"mqttServer", "MQTT Server", "", (const int) 40};
WiFiManagerParameter custom_mqtt_port("mqttPort", "MQTT Port", "", 6);
WiFiManagerParameter custom_mqtt_user = {"mqttUser", "MQTT User", "", (const int) 40};
WiFiManagerParameter custom_mqtt_password = {"mqttPassword", "MQTT Password", "", (const int) 40};
WiFiManagerParameter custom_mqtt_ssl = {"mqttSsl", "MQTT SSL", "false", 8, "placeholder=\"mqttssl\" type=\"checkbox\""};
WiFiManagerParameter custom_mqtt_topic = {"mqttTopic", "MQTT Topic", "", (const int) 60};
  
WiFiClient espClient;
PubSubClient mqttClient(espClient);

ESPMQTT2LIB::ESPMQTT2LIB()
{
}

void ESPMQTT2LIB::webServerCallback()
{
	ESPMQTT2LIB::wifiManager.server->on("/custom", std::bind(&ESPMQTT2LIB::handleWWWCustom, this)); 
}

void ESPMQTT2LIB::handleWWWCustom()
{
  wifiManager.server->send(200, "text/plain", "hello from user code");
}

void ESPMQTT2LIB::setup()
{
	systemTimer.attach_ms(100, std::bind(&ESPMQTT2LIB::systemTimerCallback, this));

	EEPROM.begin(512);

	char hostname[50];
	snprintf(hostname, 50, "ESPMQTT2_ %08X", ESP.getChipId());
	DEBUG_I("%s", hostname);
	this->sethostname(hostname);

    WiFi.setSleepMode(WIFI_NONE_SLEEP); // When sleep is on regular disconnects occur https://github.com/esp8266/Arduino/issues/5083
	WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP 
	wifiManager.addParameter(&custom_mqtt_server);
	wifiManager.addParameter(&custom_mqtt_port);
	wifiManager.addParameter(&custom_mqtt_user);
	wifiManager.addParameter(&custom_mqtt_password);
	wifiManager.addParameter(&custom_mqtt_ssl);
	wifiManager.setSaveParamsCallback(wm_saveParamsCallback);
	wifiManager.setWebServerCallback(std::bind(&ESPMQTT2LIB::webServerCallback, this));


  	// wifiManager.resetSettings();
	wifiManager.setDebugOutput(true);
  	wifiManager.setHostname(hostname);
//  	wifiManager.setEnableConfigPortal(true);
  	wifiManager.setConfigPortalBlocking(false);
  	wifiManager.autoConnect();
	wifiManager.setClass("invert");

 	std::vector<const char *> menu = {"wifi","info","param","custom","sep","restart"};
  	wifiManager.setMenu(menu);
	const char* menuhtml = "<form action='/custom' method='get'><button>Custom</button></form><br/>\n";
  	wifiManager.setCustomMenuHTML(menuhtml);
//	webServer = new ESP8266WebServer(80);
//	webServer->on("/", std::bind(&ESPMQTT2LIB::handleWWWroot, this));
//	webServer->onNotFound(std::bind(&ESPMQTT2LIB::handleNotFound, this));
}	

void ESPMQTT2LIB::loop()
{
	char valuestring[30];
	snprintf(valuestring, 30, "%d:%02d:%02d:%02d", uptime / 86400, (uptime / 3600) % 24, (uptime / 60) % 60, uptime % 60);


	//wm_loop();
	Debug.handle();
//	webServer->handleClient();
	
	// Check if wifi just has connected
	if (!this->wifiConnected && WiFi.isConnected())
	{
		this->wifiConnected = true;
//		mqttClient.connect(getvalue("hostname"));
		this->mqttConnecting = true;
//	  	webServer->begin();
	//	wifiManager.setHttpPort(80);
		wifiManager.startWebPortal();
		
		Debug.begin("esp");
  		Debug.setPassword("esplogin");
  		Debug.setResetCmdEnabled(true);

	}

	// Check if wifi just has disconnected
	if (this->wifiConnected && !WiFi.isConnected())
	{
		this->wifiConnected = false;
		mqttClient.disconnect();
		this->mqttConnecting = false;
		this->mqttConnected = false;
	}

	if (WiFi.isConnected())
	{
	}

	// Check if mqtt just has connected
	if (mqttClient.connected() && !this->mqttConnected)
	{
		this->mqttConnected = true;
		this->publishvalues();
	}

    wifiManager.process(); // do processing
}

void ESPMQTT2LIB::sethostname(const char* hostname)
{
	setvalue("hostname", hostname);
	wifiManager.setHostname(hostname);
}

int ESPMQTT2LIB::setvalue(const char* name, const char* value)
{
	int i;
	EspMqtt2Value *savevalue;
	for(i = 0; i < espMqtt2ValueList.size(); i++)
	{
		savevalue = espMqtt2ValueList.get(i);
		if (strcmp(name, savevalue->name) == 0)
		{
			realloc(savevalue->value, strlen(value)+1);
			strcpy(savevalue->value, value);
			return i;
		}
	}

	savevalue = new EspMqtt2Value();
	savevalue->name = (char *)malloc(strlen(name)+1);
	savevalue->value = (char *)malloc(strlen(value)+1);
	strcpy(savevalue->name, name);
	strcpy(savevalue->value, value);
	espMqtt2ValueList.add(savevalue);

	return i;
}

const char* ESPMQTT2LIB::getvalue(const char* name)
{
	EspMqtt2Value *savevalue;
	for(int i = 0; i < espMqtt2ValueList.size(); i++){

		// Get animal from list
		savevalue = espMqtt2ValueList.get(i);
		if (strcmp(name, savevalue->name) == 0)
		{
			return savevalue->value;
		}
	}
	return nullptr;
}

int ESPMQTT2LIB::publishvalue(const char* name)
{
	EspMqtt2Value *savevalue;
	for(int i = 0; i < espMqtt2ValueList.size(); i++){
		savevalue = espMqtt2ValueList.get(i);
		if (strcmp(name, savevalue->name) == 0)
		{
			mqttClient.publish (savevalue->name, savevalue->value, true);
			return i;
		}
	}
	return -1;
}


int ESPMQTT2LIB::publishvalue(const char* name, const char* value)
{
	setvalue(name, value);
	return publishvalue(name);
}

int ESPMQTT2LIB::publishvalues()
{
	EspMqtt2Value *savevalue;
	int i;
	for(i = 0; i < espMqtt2ValueList.size(); i++){
		savevalue = espMqtt2ValueList.get(i);
		mqttClient.publish (savevalue->name, savevalue->value, true);
	}
	return i;
}


void ESPMQTT2LIB::wm_loop(){
  if (!WiFi.isConnected() && !wifiManagerBootStarted && this->uptime > 30)
  {
	startWiFiManager = true;
	wifiManagerBootStarted = true;
	wm_startAP = true;
  }

  if (WiFi.isConnected())
  {
	wifiManagerBootStarted = true;
    if(wm_startAP){
  		wifiManager.stopConfigPortal();
    }
    else{
		wifiManager.stopWebPortal();
    } 
  }

  // is auto timeout portal running
  if(wm_portalRunning){
    wifiManager.process(); // do processing

    // check for timeout
    if((millis()-wm_startTime) > (wm_timeout*1000)){
      Serial.println("portaltimeout");
      wm_portalRunning = false;
      if(wm_startAP){
        wifiManager.stopConfigPortal();
      }
      else{
        wifiManager.stopWebPortal();
      } 
   }
  }

  // is configuration portal requested?
  if(startWiFiManager && !wm_portalRunning) {
	startWiFiManager = false;
    if(wm_startAP){
      Serial.println("Button Pressed, Starting Config Portal");
      wifiManager.setConfigPortalBlocking(false);
      wifiManager.startConfigPortal();
    }  
    else{
      Serial.println("Button Pressed, Starting Web Portal");
      wifiManager.startWebPortal();
    }  
    wm_portalRunning = true;
    wm_startTime = millis();
  }
}

void ESPMQTT2LIB::wm_saveParamsCallback () {
  Serial.println("Get Params:");
  Serial.print(custom_mqtt_server.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_server.getValue());
}

void ESPMQTT2LIB::systemTimerCallback()
{
  static uint8_t ms = 0;
  ms++;
  if (ms >= 10)
  {
    this->uptime++;
    ms = 0;
  }
}

void ESPMQTT2LIB::setSerialDebug(bool enable)
{
	this->Debug.setSerialEnabled(enable);
	wifiManager.setDebugOutput(enable);
}
/*
void ESPMQTT2LIB::handleWWWroot()
{
	//webServer->send(200, "text/plain", "Hello World!"); 
}

void ESPMQTT2LIB::handleNotFound()
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += webServer->uri();
	message += "\nMethod: ";
	message += (webServer->method() == HTTP_GET)?"GET":"POST";
	message += "\nArguments: ";
	message += webServer->args();
	message += "\n";
	for (uint8_t i=0; i< webServer->args(); i++)
	{
	message += " " + webServer->argName(i) + ": " + webServer->arg(i) + "\n";
	}
	webServer->send(404, "text/plain", message);
}*/