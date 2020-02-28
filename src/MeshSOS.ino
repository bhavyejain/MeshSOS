/*
 * Project: MeshSOS
 * Description: A mesh networking based SOS support system for senior citizens
 * Authors: Bhavye Jain, Kaustubh Trivedi, Twarit Waikar
 * Date: 28 January 2020
 */

#include "JsonParserGeneratorRK.h"
#include "google-maps-device-locator.h"

// variables for I/O
int button_med = A1;
int button_pol = A2;
int ledPin = D7;

// variables to store input values
int val_med = 0;
int val_pol = 0;

// cloud variables
int btn_medical;
int btn_police;

// strings to build the main SOS message
const String DEVICE = System.deviceID();
String MESSAGE_MEDICAL = "medical-";
String MESSAGE_POLICE = "police-";

// device geolocation
GoogleMapsDeviceLocator locator;
String latitude = "-1";       // setting default to -1 to ease error detection later
String longitude = "-1";
String accuracy = "-1";

// variables for WiFi toggle (testing)
int wifi_btn = A5;
int val_wifi = 0;
bool wifi_flag = true;

void setup() {
  
  pinMode(button_med, INPUT_PULLDOWN);  // take input from medical emergency button
  pinMode(button_pol, INPUT_PULLDOWN);  // take input from police emergency button
  pinMode(wifi_btn, INPUT_PULLDOWN);    // take input from wifi toggle button

  pinMode(ledPin, OUTPUT);  // LED output for testing

  Particle.variable("medical", btn_medical);  // make btn_medical visible to cloud
  Particle.variable("police", btn_police);  // make btn_police visible to cloud

  Particle.subscribe("hook-response/emergency", hookResponseHandler, ALL_DEVICES);
  Mesh.subscribe("m_emergency", meshEmergencyHandler);

  digitalWrite(ledPin, LOW);    // keep LED off on setup

  // augment emergency messages with device ID
  MESSAGE_MEDICAL.concat(DEVICE);
  MESSAGE_POLICE.concat(DEVICE);

  locator.withSubscribe(locationCallBack);
  locator.publishLocation();

  Serial.begin(9600);   // initialize serial output ($ particle serial monitor)
  Serial.print("Device: "); Serial.println(DEVICE); // print device ID
  Serial.print("Location::  "); Serial.println("lat: " + String(latitude) + " lon: " + String(longitude) + "  acc: " + String(accuracy));
}


void loop() {

  // read inputs from respective buttons
  val_med = digitalRead(button_med);  
  val_pol = digitalRead(button_pol);  
  
  // update values of cloud-visible variables
  btn_medical = val_med;
  btn_police = val_pol;

  if(val_med == 1){
    locator.publishLocation();    // update device location
    String payload = createEventPayload(MESSAGE_MEDICAL, latitude, longitude, accuracy);    // create JSON object with all required data to be sent

    if(Particle.connected()){     // if the device is connected to the cloud, directly publish message to the cloud
      publishToCloud("emergency/medical", payload);
    }
    else{   // publish to mesh
      publishToMesh("m_emergency/medical", payload);
    }

    delay(500);   // do not publish multiple times for a long press
  }
  if(val_pol == 1){
    locator.publishLocation();    // update device location
    String payload = createEventPayload(MESSAGE_POLICE, latitude, longitude, accuracy);     // create JSON object with all required data to be sent

    if(Particle.connected()){     // if the device is connected to the cloud, directly publish message to the cloud
      publishToCloud("emergency/police", payload);
    }
    else{   // publish to mesh
      publishToMesh("m_emergency/police", payload);
    }

    delay(500);   // do not publish multiple times for a long press
  }

  // toggle wifi for testing
  val_wifi = digitalRead(wifi_btn);
  if(val_wifi == 1){
    toggleWifi();

    delay(500);
  }
}

// create JSON data
char* createEventPayload(String emergency, String lat, String lon, String acc){      

  JsonWriterStatic<512> jw;
  {
    JsonWriterAutoObject obj(&jw);

    jw.insertKeyValue("emergency", emergency);
    jw.insertKeyValue("latitude", lat);
    jw.insertKeyValue("longitude", lon);
    jw.insertKeyValue("accuracy", acc);
  }

  return jw.getBuffer();
}

// publish the emergency message to Particle cloud
bool publishToCloud(String filter, String message){       

  // run the loop if publish() returns false => failed publish
  while(!Particle.publish(filter, message)){    // keep trying to publish until successful
    delay(1000);   // wait for 1 second (allowed publish rate is per second)
  }

  // print to serial output
  Serial.print("PUBLISH_TO_CLOUD :: ");
  Serial.println(message);
}

// broadcast emergency message in the mesh
bool publishToMesh(String filter, String message){      

  int attempts = 0;
  while(!Mesh.ready()){     // if device not connected to the mesh, try to connect 3 times
    Mesh.connect();
    ++attempts;
    if(attempts >= 3)
      break;
  }

  if(Mesh.ready()){
    int pub = Mesh.publish(filter, message);
    // run the loop if publish() returns non 0 value => failed publish
    while(pub != 0){    // keep trying to publish until successful
      pub = Mesh.publish(filter, message);
      delay(1000);    // wait for 1 second (allowed publish rate is per second)
    }

    Serial.print("PUBLISH_TO_MESH :: ");
    Serial.println(message);
  }
  else{       // unable to publish (device not connected to mesh)
    Serial.print("FAILED_PUBLISH_TO_MESH :: ");
    Serial.println(message);
  }
}

// handle emergency calls received through mesh
void meshEmergencyHandler(const char *event, const char *data){     
  Serial.print("MESH_RECEIVED:: event: ");
  Serial.print(event);
  Serial.print(", data: ");
  if(data){
    Serial.println(data);
  }
  else{
    Serial.println("NULL");
  }

  String filter = event;
  String message = data;

  int i = filter.indexOf('/');
  String e_type = filter.substring((i+1));    // get the emergency type

  if(e_type.equals("medical")){

    if(Particle.connected()){     // if the device is connected to the cloud, publish message to the cloud
      publishToCloud("emergency/medical", message);
    }
  }
  if(e_type.equals("police")){

    if(Particle.connected()){     // if the device is connected to the cloud, publish message to the cloud
      publishToCloud("emergency/police", message);
    }
  }
}

// handle responses from the server (via webhook)
void hookResponseHandler(const char *event, const char *data){     
  // TODO : send ack to SOS callee device
}

// function to extract the device ID from received augmented message
String getDeviceID(String data){    

  int i = data.indexOf('-');
  String devID = data.substring(i+1).trim();
  return devID;
}

// handle the response by Google Maps API
void locationCallBack(float lat, float lon, float acc){     
  latitude = String(lat);
  longitude = String(lon);
  accuracy = String(acc);
}

void toggleWifi(){
  if(wifi_flag){
    WiFi.disconnect();
    wifi_flag = false;
  }
  else{
    WiFi.connect();
    wifi_flag = true;
  }
}