#include <Arduino.h>
#include <WiFiManager.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <Preferences.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "FS.h"
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true

#include "multiboot.hpp"

#define LED_PIN 21

size_t data_len = 0;

uint8_t led_state = 0;

WiFiClient wiFiClient;
WiFiManager	wifiManager;

File file;

void File_Upload() {
	String webpage = "";
	webpage += FPSTR(HTTP_STYLE);
	webpage += F("<body class='invert'><div class='wrap'>");
	webpage += F("<h3>Select File to upload an image to display</h3>"); 
	webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
	webpage += F("<input class='buttons' type='file' name='fupload' id = 'fupload' value=''><br>");
	webpage += F("<br><button class='buttons' type='submit'>Upload File</button><br>");
	webpage += F("</div'></body'>");
	wifiManager.server->send(200, "text/html", webpage);
}

void handleFileUpload(){ // upload a new file to the Filing system
	String webpage = "";
	HTTPUpload& uploadfile = wifiManager.server->upload();
	if(uploadfile.status == UPLOAD_FILE_START) {
		Serial.print("Upload File Name: ");
		Serial.println(uploadfile.filename);
		Serial.print("type: ");
		Serial.println(uploadfile.type);

		digitalWrite(LED_PIN, HIGH);

		data_len = 0;
		file = SPIFFS.open("/buffer.data", FILE_WRITE);

	} else if (uploadfile.status == UPLOAD_FILE_WRITE) {
		Serial.print("Write: ");
		Serial.println(uploadfile.currentSize);

		file.write(uploadfile.buf, uploadfile.currentSize);

		data_len += uploadfile.currentSize;
	}
	else if (uploadfile.status == UPLOAD_FILE_END) {
		Serial.print("END: ");
		Serial.println(data_len);
		digitalWrite(LED_PIN, LOW);

		file.close();

		File file = SPIFFS.open("/buffer.data", FILE_READ);
		multiboot(file, data_len);
		file.close();

		webpage = "";
		webpage += FPSTR(HTTP_STYLE);
		webpage += F("<body class='invert'><div class='wrap'>");
		webpage += F("<h3>File was successfully uploaded</h3>"); 
		webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
		webpage += F("<a href='/upload'>[Back]</a><br><br>");
		webpage += F("</div'></body'>");
		wifiManager.server->send(200,"text/html", webpage);
	}
}

void setup() {

	Serial.begin(115200);
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);
	WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

	if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
		Serial.println("SPIFFS Mount Failed");
		return;
	}

	#ifdef BOARD_HAS_PSRAM
		Serial.printf("esp32 use external RAM\n");
	#else
		Serial.printf("esp32 use internal RAM\n");
	#endif

	Serial.printf("Start Wifi manager\n");
	wifiManager.setDebugOutput(true);
	wifiManager.setTimeout(180);
	wifiManager.setConfigPortalTimeout(180); // try for 3 minute
	wifiManager.setMinimumSignalQuality(15);
	wifiManager.setRemoveDuplicateAPs(true);

	wifiManager.setClass("invert"); // dark theme

	std::vector<const char*> menu = {"wifi", "info", "sep", "update", "restart" };
	wifiManager.setMenu(menu);

	wifiManager.setHostname("gba_wifi");

	bool rest = wifiManager.autoConnect("GBA_AP");

	if (rest) {
		Serial.println("Wifi connected");
		wifiManager.startWebPortal();

		wifiManager.server->on("/upload",   File_Upload);
		wifiManager.server->on("/fupload",  HTTP_POST,[](){ wifiManager.server->send(200);}, handleFileUpload);
	}
	else
		ESP.restart();
	Serial.print("IP: "); Serial.println(WiFi.localIP());

	Serial.print("Setup task running on: ");
	Serial.println(xPortGetCoreID());

	digitalWrite(LED_PIN, HIGH);
	delay(500);
	digitalWrite(LED_PIN, LOW);
}

unsigned long next_frame = 0;

void loop() {
	wifiManager.process();
	vTaskDelay(10 / portTICK_PERIOD_MS);
}