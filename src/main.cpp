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


extern "C" {
    #include "multiboot.h"
}

uint8_t *data;
size_t data_len = 0;

WiFiClient wiFiClient;
WiFiManager	wifiManager;

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
		Serial.print("Upload File Name: "); Serial.println(uploadfile.filename);
		Serial.print("type: "); Serial.println(uploadfile.type);

		#ifdef USE_LED
			digitalWrite(led, HIGH);
		#endif

		if (data)
			free(data);
		data = (uint8_t*)malloc(1);
		data_len = 0;

	} else if (uploadfile.status == UPLOAD_FILE_WRITE) {
		Serial.print("Write: "); Serial.println(uploadfile.currentSize);
		uint8_t* ptr_tmp = (uint8_t*)realloc(data, data_len + uploadfile.currentSize);
		data = ptr_tmp;
		memcpy(data + data_len, uploadfile.buf, uploadfile.currentSize);
		data_len += uploadfile.currentSize;
	}
	else if (uploadfile.status == UPLOAD_FILE_END) {
		Serial.print("END: "); Serial.println(data_len);
		multiboot(data, data_len);
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
	pinMode(21, OUTPUT);
	digitalWrite(21, 1);
	WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);

	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

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
}

unsigned long next_frame = 0;

void loop() {
	wifiManager.process();
	vTaskDelay(10 / portTICK_PERIOD_MS);
}