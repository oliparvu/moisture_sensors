#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define HOME 0
#define OUT 1

#define SENSOR 3
#define LOCATION HOME



constexpr auto MQTT_SERVER = "oliparvu.go.ro";
constexpr auto MQTT_PORT = 1883;
#define mqtt_user "123"      // if exist
#define mqtt_password "123"  //idem

#define GPIO2 2
#define GPIO0 0
#define GPIO4 4


#define SENSOR_SUPPLY GPIO4
#define SECONDS 1000000
#define VOLTAGE_DIVIDER_RATIO 3

#if SENSOR == 1
#define device_name  "sensor01"
#define small_device_name "node01" //6characters small name
constexpr char Topic[50] = "main/dressing/moisture/sensor01";
#elif SENSOR == 2
#define device_name  "sensor02"
#define small_device_name "node02" //6characters small name
constexpr char Topic[50] = "main/dressing/moisture/sensor02";
#elif SENSOR == 3
#define device_name  "sensor03"
#define small_device_name "node03" //6characters small name
constexpr char Topic[50] = "main/dressing/moisture/sensor03";
#endif

#if LOCATION == OUT
constexpr auto wifi_ssid = "Oli";
constexpr auto wifi_password = "marlboro";
#elif LOCATION == HOME
constexpr auto wifi_ssid = "Chillout";
constexpr auto wifi_password = "marlboro";
#endif


typedef struct SensorData_t {
	uint16_t uwAdcVal;
	float fVoltage;
	float fComputedValue;
};

//SensorData_t stSensorData;


char sensor_tele[50], state_tele[50];

void GetSensorData(SensorData_t* pstSenData);
void send_mqtt_data(SensorData_t* pstSenData);

WiFiClient espClient;
PubSubClient client(espClient);

#if SENSOR == 2
ADC_MODE(ADC_VCC);
#endif
#if SENSOR == 3
ADC_MODE(ADC_VCC);
#endif

void setup()
{

	pinMode(SENSOR_SUPPLY, OUTPUT);
	digitalWrite(SENSOR_SUPPLY, LOW);

	strcpy(sensor_tele, device_name);
	strcat(sensor_tele, "/tele/sensor");
	strcpy(state_tele, device_name);
	strcat(state_tele, "/tele/state");


	Serial.begin(115200);
	setup_wifi();           //Connect to Wifi network

	client.setServer(MQTT_SERVER, MQTT_PORT);    // Configure MQTT connexion



}
void loop()
{
	SensorData_t stSensorData;

	if (!client.connected())
	{
		reconnect();
	}
	client.loop();

	delay(50);

	GetSensorData(&stSensorData);
	send_mqtt_data(&stSensorData);


#if SENSOR == 1
	Serial.print("entering deep sleep");
	ESP.deepSleep(600 * SECONDS);
#elif SENSOR == 2
#elif SENSOR == 3
#endif
	delay(500);
}


//Connexion au réseau WiFi
void setup_wifi()
{
	delay(10);
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(wifi_ssid);

	WiFi.begin(wifi_ssid, wifi_password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi OK ");
	Serial.print("=> ESP8266 IP address: ");
	Serial.println(WiFi.localIP());
}

//Reconnexion
void reconnect()
{
	String clientId = small_device_name;
	char espname[14];
	while (!client.connected()) {
		Serial.print("Connecting to MQTT broker ...");
		clientId += "-";
		clientId += String(random(0xffff), HEX);
		Serial.printf("MQTT connecting as client %s...\n", clientId.c_str());
		clientId.toCharArray(espname, 12);
		if (client.connect(espname, mqtt_user, mqtt_password)) {
			Serial.println("OK");
		}
		else {
			Serial.print("KO, error : ");
			Serial.print(client.state());
			Serial.println(" Wait 5 secondes before to retry");
			delay(5000);
		}
	}
}



void GetSensorData(SensorData_t *pstSenData)
{
#if SENSOR == 1

	Serial.println("PowerON Sensor");
	digitalWrite(SENSOR_SUPPLY, HIGH); //turn on sensor
	delay(1000);//settling time
	pstSenData->uwAdcVal = analogRead(A0);  //get sesnor measurement
	digitalWrite(SENSOR_SUPPLY, LOW); //turn on sensor
	pstSenData->fVoltage = ((float)pstSenData->uwAdcVal / 1024.0f) * VOLTAGE_DIVIDER_RATIO;//

#elif SENSOR == 2
	pstSenData->uwAdcVal =  ESP.getVcc();  //get sesnor measurement
	pstSenData->fVoltage = (float)pstSenData->uwAdcVal / 1024.0f;
#elif SENSOR == 3
	pstSenData->uwAdcVal = ESP.getVcc();  //get sesnor measurement
	pstSenData->fVoltage = (float)pstSenData->uwAdcVal / 1024.0f ;
#endif

	
	pstSenData->fComputedValue = ComputeMoisturePercentage(pstSenData->uwAdcVal);

}

void send_mqtt_data(SensorData_t* pstSenData)
{
	/*
	char szRawValue[50];
	char szSensorVoltage[50];
	char szPercentage[50];
	*/
	char JSONmessageBuffer[100];
	   	  
	StaticJsonBuffer<600> JSONbuffer;
	JsonObject& JSONencoder = JSONbuffer.createObject();

	JSONencoder["voltage"] = pstSenData->fVoltage;
	JSONencoder["RAW"] = pstSenData->uwAdcVal;
	JSONencoder["Moisture_Percent"] = pstSenData->fComputedValue;

	JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

	Serial.print("JSON: ");

	client.publish(Topic, JSONmessageBuffer, true);

	Serial.println(JSONmessageBuffer);


	Serial.println();

}

uint8_t ComputeMoisturePercentage(const uint16_t uwInRaw)
{//WIP
	uint16_t uwValueToConvert;
	uint8_t ubRetVal;

	Serial.println("Start");
	Serial.println(uwInRaw);
	if (uwInRaw >= 1000) //very dry case. max recorded in air was ~900
	{
		uwValueToConvert = 1000;
	}
	else
	{
		if (uwInRaw < 500)//max value obtained in water. no point going below.
		{
			uwValueToConvert = 500;
		}
		else
		{
			uwValueToConvert = uwInRaw;
		}

	}
	Serial.println(uwValueToConvert);
	ubRetVal = 100 - (uwValueToConvert / 10);
	Serial.println(ubRetVal);
	return ubRetVal;
	
}