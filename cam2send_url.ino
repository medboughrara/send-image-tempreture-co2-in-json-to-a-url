#include <HTTPClient.h>
#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"     
#include <EEPROM.h>           
#include <WiFi.h>             
#include <WebServer.h>  
#include <Base64.h> 
#include <DHT.h>



const char* ssid = "wifi name"; 
const char* password = "pass";    
WebServer server(80);                    


// DHT11 Configuration
// #define DHTPIN 16             // Digital pin connected to the DHT11 data pin
//#define DHTTYPE DHT11        // DHT 11

//DHT dht(DHTPIN, DHTTYPE);      // Create an instance of the DHT class

// MQ-811 Configuration
//#define MQ811_PIN 34         // Analog pin connected to the MQ-811 output



int pictureNumber = 0;

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22






// New endpoint for JSON response
void handleJson() {
  camera_fb_t * fb = NULL;
  digitalWrite(4,HIGH);
  // Take Picture with Camera
  fb = esp_camera_fb_get();   
  if (!fb) { 
    Serial.println("Camera capture failed"); 
    return; 
  }
  delay(1000);
  digitalWrite(4,LOW);
  // Initialize EEPROM with predefined size 
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;

  // Convert JPEG image to Base64
  String imageBase64 = base64::encode(fb->buf, fb->len);

  // Generate random float values
  float temp = random(240, 250) / 10.0; // 24.0 to 25.0
  float co2c = random(600, 650);        // 600 to 650

  Serial.printf("temp: %.2f °C, co2c: %.2f\n", temp, co2c);

  // Properly format the JSON string
 String jsonResponse = "{\"camera\": {\"value\":\"" + String(imageBase64) + "\"}," +
                      "\"temp\": {\"value\":" + String(temp) + "}," +
                      "\"co2\": {\"value\":" + String(co2c) + "}}";
  // Send the JSON response
  server.send(200, "application/json", jsonResponse);
  Serial.println(jsonResponse); // Print the entire JSON response for debugging

  // Free up the frame buffer
  esp_camera_fb_return(fb);

  // Send JSON to a defined external URL
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://tecxendpoint-4iu9kze6.b4a.run/report/";
    http.begin(url);
    http.addHeader("Content-Type", "application/json"); 

    // Send the JSON data
    int httpResponseCode = http.POST(jsonResponse);

    // Check HTTP response
    if (httpResponseCode > 0) {
      String response = http.getString(); // Get response from server
      Serial.println(httpResponseCode);   // Print the response code
      Serial.println(response);           // Print the response
    } else {
      Serial.printf("Error sending POST request: %d\n", httpResponseCode);
    }

    http.end(); // Close the connection
  } else {
    Serial.println("WiFi not connected");
  }
}




void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout 
  pinMode(4, OUTPUT);
  //dht.begin();
  Serial.begin(115200); 
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); 
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP()); 


  server.on("/json", handleJson); // New endpoint for JSON response
  server.begin(); 
  Serial.println("HTTP server started");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0; 
  config.pin_d0 = Y2_GPIO_NUM; 
  config.pin_d1 = Y3_GPIO_NUM; 
  config.pin_d2 = Y4_GPIO_NUM; 
  config.pin_d3 = Y5_GPIO_NUM; 
  config.pin_d4 = Y6_GPIO_NUM; 
  config.pin_d5 = Y7_GPIO_NUM; 
  config.pin_d6 = Y8_GPIO_NUM; 
  config.pin_d7 = Y9_GPIO_NUM; 
  config.pin_xclk = XCLK_GPIO_NUM; 
  config.pin_pclk = PCLK_GPIO_NUM; 
  config.pin_vsync = VSYNC_GPIO_NUM; 
  config.pin_href = HREF_GPIO_NUM; 
  config.pin_sscb_sda = SIOD_GPIO_NUM; 
  config.pin_sscb_scl = SIOC_GPIO_NUM; 
  config.pin_pwdn = PWDN_GPIO_NUM; 
  config.pin_reset = RESET_GPIO_NUM; 
  config.xclk_freq_hz = 20000000; 
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){ 
    config.frame_size = FRAMESIZE_UXGA; 
    config.jpeg_quality = 10; 
    config.fb_count = 2; 
  } else {
    config.frame_size = FRAMESIZE_SVGA; 
    config.jpeg_quality = 12; 
    config.fb_count = 1; 
  }
  
  // Init Camera
  esp_err_t err = esp_camera_init(&config); 
  if (err != ESP_OK) { 
    Serial.printf("Camera init failed with error 0x%x", err); 
    return; 
  }
  
  Serial.println("Camera initialized"); // ( this line for clarity)
}

void loop() {
  server.handleClient(); 
  handleJson();
  delay(29000);
}
