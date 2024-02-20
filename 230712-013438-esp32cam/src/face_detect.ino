#include "esp_camera.h"
#include "fd_forward.h"
#include <WiFi.h>
#include <Arduino.h>
#include "PubSubClient.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"

const char* ssid = "HelloWorld";
const char* wifipass = "12344321";

const char* mqtt_server = "nightkoala963.cloud.shiftr.io";
const unsigned int mqtt_port = 1883;
#define MQTT_USER               "nightkoala963"
#define MQTT_PASSWORD           "ijfMV2BbIpZgvPBG"
#define MQTT_PUBLISH_TOPIC    "Mytopic/send"
#define MQTT_SUBSCRIBE_TOPIC    "active"

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

bool active = false;
WiFiClient espClient;
PubSubClient client(espClient);

bool initCamera() {
 
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
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;
 
  esp_err_t result = esp_camera_init(&config);
 
  if (result != ESP_OK) {
    return false;
  }
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);
  return true;
}
 
mtmn_config_t mtmn_config = {0};
int detections = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[");
  Serial.print(topic);
  Serial.print("] ");
  String temp="";
  for (int i = 0; i < length; i++) {
    temp += String((char)payload[i]);
  }
  if(temp == "1") active = true;
  else if(temp == "0") active = false;
  Serial.println();
}

void WifiSetup(){

  Serial.println("Start connecting to " + String(ssid));
  WiFi.begin(ssid,wifipass);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println("Wifi connected");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-cam";
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish(MQTT_PUBLISH_TOPIC, "hello world");
      // ... and resubscribe
      client.subscribe(MQTT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  if (!initCamera()) {
    Serial.printf("Failed to initialize camera...");
    ESP.restart();
    return;
  }
  delay(500);
  WifiSetup();
  delay(10);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setBufferSize(2048);
  
  mtmn_config = mtmn_init_config();
}

String sendImage(camera_fb_t * fb) {
  char *input = (char *)fb->buf;
  char output[base64_enc_len(3)];
  //char output[base64_enc_len(3)];
  String imageFile = "";
  for (int i=0;i<fb->len;i++) {
    base64_encode(output, (input++), 3);
    
    if (i%3==0) imageFile += String(output);
  }
  int fbLen = imageFile.length();
  
  String clientId = "ESP32-cam";
  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    client.beginPublish(MQTT_PUBLISH_TOPIC, fbLen, false);

    String str = "";
    for (size_t n=0;n<fbLen;n=n+2048) {
      if (n+2048<fbLen) {
        str = imageFile.substring(n, n+2048);
        client.write((uint8_t*)str.c_str(), 2048);
      }
      else if (fbLen%2048>0) {
        size_t remainder = fbLen%2048;
        str = imageFile.substring(n, n+remainder);
        client.write((uint8_t*)str.c_str(), remainder);
      }
    }  
    
    client.endPublish();
    
    esp_camera_fb_return(fb);
    
    return "";
  }
  esp_camera_fb_return(fb);
  return "failed, rc="+client.state();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(active){
    camera_fb_t * frame = esp_camera_fb_get();
  
    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, frame->width, frame->height, 3);
    fmt2rgb888(frame->buf, frame->len, frame->format, image_matrix->item);
  
    box_array_t *boxes = face_detect(image_matrix, &mtmn_config);
    dl_matrix3du_free(image_matrix);
  
    if (boxes != NULL) {
      detections = detections+1;
      Serial.printf("Faces detected %d times \n", detections);
    // camera_fb_t * frame;
    // frame = esp_camera_fb_get();
      if(detections > 3) {
        sendImage(frame);
        active = false;
        detections = 0;
      }
      dl_lib_free(boxes->score);
      dl_lib_free(boxes->box);
      dl_lib_free(boxes->landmark);
      dl_lib_free(boxes);
    }
  }
 
}