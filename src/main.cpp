/*
 * Smart Sign Language Translator Glove
 * Authors: Bhavjot Gill & Arvind Dhaliwal
 * Date: 01/12/2025
 *
 * This device utilizes five flex sensors and an MPU-6050 for accelerometer and gyroscope readings.
 * The collected data is processed through a K-Nearest Neighbors (KNN) machine learning model
 * to accurately predict and translate sign language gestures in real-time.
 */

#include <Arduino.h>
#include <cmath>
#include <algorithm>
#include <array>
#include <iostream>
#include <fstream>
#include <vector>
#include <LittleFS.h>
#include <WiFi.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Arduino_JSON.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "LittleFS.h"
#include <map>
#include <unordered_map>

// Timer variables
unsigned long lastTime = 0;
unsigned long lastTimeTemperature = 0;
unsigned long lastTimeAcc = 0;
unsigned long gyroDelay = 50;
unsigned long temperatureDelay = 1000;
unsigned long accelerometerDelay = 200;
unsigned long delay1 = 10;
int counter = 0;

// Set variables for feature scaling
const float FLEX_MIN = 0.0;
const float FLEX_MAX = 1.5;
const float GYRO_MIN = -250.0;
const float GYRO_MAX = 250.0;
const float ACCEL_MIN = -16.0;
const float ACCEL_MAX = 16.0;

// Create a sensor object for MPU6050
Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;
float gyroX, gyroY, gyroZ;
float accX, accY, accZ;
float temperature;
using namespace std;
// Gyroscope sensor deviation
float gyroXerror = 0.17;
float gyroYerror = 0.03;
float gyroZerror = 0.01;

// Connect to your Wi-Fi network using id and password
const char *ssid = "";
const char *password = "";

// Create an instance of the web server
AsyncWebServer server(80);
AsyncEventSource events("/events");
JSONVar readings;

// Initialize pins and variables for flexor sensors
const int flexorpin = 36;
const int flexorpin2 = 39;
const int flexorpin3 = 34;
const int flexorpin4 = 35;
const int flexorpin5 = 32;
float flexorval = 0.0;
float flexorval2 = 0.0;
float flexorval3 = 0.0;
float flexorval4 = 0.0;
float flexorval5 = 0.0;

// Init MPU6050
void initMPU()
{
  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1)
    {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
}

// Struct to store sensor values and label of the data
struct DataCSV_ML
{
  float Avals[132];
  std::string label;
};
// Struct to store all trained values from csv file
DataCSV_ML Y_Traind[50];
const int Y_Traind_RowSize = 50;
const char *filename = "/SensorData.csv";

// mode 0-> Training mode (Train the KNN model)
// mode 1-> Running mode (Sign language translation)
// mode 3-> Free play mode (Threejs hand free play)
int mode = 0;
// Initialize k for KNN model
int k = 4;

// Create a queue to track each value every 250ms(delay)from the sensors and hold it for QUEUE_SIZE spots
//(ex. with a delay of 250 and queue size of 12 we can store values up to 3 seconds )
#define QUEUE_SIZE 12
class Queue
{
private:
  float queue[QUEUE_SIZE];
  int front;
  int rear;
  int count;
  float Cvalue;

public:
  Queue()
  {
    front = 0;
    rear = -1;
    count = 0;
    Cvalue = 0.0;
  }

  // Enqueue an element at the rear
  bool enqueue(float value)
  {
    if (count >= QUEUE_SIZE)
    {
      Serial.println("Queue Overflow");
      return false;
    }
    rear = (rear + 1) % QUEUE_SIZE;
    queue[rear] = value;
    count++;
    return true;
  }

  // Dequeue an element from the front
  float dequeue()
  {
    if (count <= 0)
    {
      Serial.println("Queue Underflow");
      return -1.0; // Return -1.0 if the queue is empty
    }
    float value = queue[front];
    front = (front + 1) % QUEUE_SIZE;
    count--;
    return value;
  }

  // Peek at the front element without removing it
  float peek()
  {
    if (count <= 0)
    {
      Serial.println("Queue is Empty");
      return -1.0;
    }
    return queue[front];
  }

  // Check if the queue is empty
  bool isEmpty()
  {
    return count == 0;
  }

  // Check if the queue is full
  bool isFull()
  {
    return count == QUEUE_SIZE;
  }

  // Get the average of the queue elements
  float getCval()
  {
    if (count == 0)
    {
      Serial.println("Queue is Empty");
      return 0.0;
    }

    Cvalue = 0.0;
    for (int i = 0; i < count; i++)
    {
      int index = (front + i) % QUEUE_SIZE;
      Cvalue += queue[index];
    }
    Cvalue /= count;
    Serial.print(Cvalue);
    Serial.print(",");
    return Cvalue;
  }

  // Get the value at the ith spot in the queue
  float getAt(int i)
  {
    if (i < 0 || i >= count)
    {
      Serial.println("Index out of range");
      return -1.0; // Error code for invalid index
    }
    int index = (front + i) % QUEUE_SIZE;
    return queue[index];
  }
  int getCount()
  {
    return count;
  }
};
// Initilize all queues for all sensors values
Queue myQueueFlex1;
Queue myQueueFlex2;
Queue myQueueFlex3;
Queue myQueueFlex4;
Queue myQueueFlex5;
Queue myQueueGyrox;
Queue myQueueGyroy;
Queue myQueueGyroz;
Queue myQueueAccelx;
Queue myQueueAccely;
Queue myQueueAccelz;
// Normalize function for KNN model
float normalize(float value, float min, float max, float target_min, float target_max)
{
  // Perform linear scaling normalization also known as min-max normalization
  return (value - min) / (max - min) * (target_max - target_min) + target_min;
}

// Load the trained data from the csv file
void loadDataset()
{
  File file = LittleFS.open(filename, "r");

  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  int row = 0;
  while (file.available() && row < Y_Traind_RowSize)
  {
    String line = file.readStringUntil('\n');
    line.trim();

    int col = 0;
    char *token = strtok(const_cast<char *>(line.c_str()), ",");
    while (token != nullptr && col < 133) // 132 data columns + 1 label column
    {
      if (col < 132)
      {
        Y_Traind[row].Avals[col] = atof(token); // Store float data
      }
      else
      {
        Y_Traind[row].label = std::string(token); // Store label
      }
      token = strtok(nullptr, ",");
      col++;
    }
    row++;
  }
  file.close();
}

void resetQueues()
{
  // Dequeue all elements from each queue
  while (!myQueueFlex1.isEmpty())
    myQueueFlex1.dequeue();
  while (!myQueueFlex2.isEmpty())
    myQueueFlex2.dequeue();
  while (!myQueueFlex3.isEmpty())
    myQueueFlex3.dequeue();
  while (!myQueueFlex4.isEmpty())
    myQueueFlex4.dequeue();
  while (!myQueueFlex5.isEmpty())
    myQueueFlex5.dequeue();
  while (!myQueueGyrox.isEmpty())
    myQueueGyrox.dequeue();
  while (!myQueueGyroy.isEmpty())
    myQueueGyroy.dequeue();
  while (!myQueueGyroz.isEmpty())
    myQueueGyroz.dequeue();
  while (!myQueueAccelx.isEmpty())
    myQueueAccelx.dequeue();
  while (!myQueueAccely.isEmpty())
    myQueueAccely.dequeue();
  while (!myQueueAccelz.isEmpty())
    myQueueAccelz.dequeue();
  // Reset each queue to fill with zeros
  for (int i = 0; i < QUEUE_SIZE; i++)
  {
    myQueueFlex1.enqueue(0);
    myQueueFlex2.enqueue(0);
    myQueueFlex3.enqueue(0);
    myQueueFlex4.enqueue(0);
    myQueueFlex5.enqueue(0);
    myQueueGyrox.enqueue(0);
    myQueueGyroy.enqueue(0);
    myQueueGyroz.enqueue(0);
    myQueueAccelx.enqueue(0);
    myQueueAccely.enqueue(0);
    myQueueAccelz.enqueue(0);
  }
}

void setup()
{
  delay(1000);
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  initMPU();
  // Initialize LittleFS
  if (!LittleFS.begin(true))
  { // true will format if LittleFS is not initialized
    Serial.println("LittleFgiled");
    return;
  }

  // Serve the HTML, CSS, and JS files from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/style.css", "text/css"); });

  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/main.js", "application/javascript"); });
  server.on("/hand.glb", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/hand.glb", "model/gltf-binary"); });

  server.on("/3.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/3.jpg", "image/jpeg"); });
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                gyroX=0;
                gyroY=0;
                gyroZ=0;
    request->send(200, "text/plain", "OK"); });
  server.on("/predict", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("working");
              mode=1;
              counter=0;
              gyroDelay = 250;
              loadDataset();
              resetQueues();
    request->send(200, "text/plain", "OK"); });
  events.onConnect([](AsyncEventSourceClient *client)
                   {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000); });
  server.addHandler(&events);
  // Start the server
  server.begin();

  if (mode == 1)
  {
    loadDataset();
  }
  resetQueues();
}

// KNN function to predict out of sign language
void GetKNN(int k, DataCSV_ML newData)
{
  // Makes pairs if distance, index and label
  std::vector<std::pair<float, std::pair<int, std::string>>> distances;

  // Loop through trained data and calculate euclidean distance
  for (int i = 0; i < Y_Traind_RowSize; i++)
  {
    float distance = 0;
    for (int j = 0; j < 132; j++)
    {
      float diff = newData.Avals[j] - Y_Traind[i].Avals[j];
      distance += diff * diff;
    }
    distances.push_back(std::make_pair(distance, std::make_pair(i, Y_Traind[i].label)));
  }

  std::partial_sort(
      distances.begin(),     // Start of the range to be sorted
      distances.begin() + k, // End of the range to partially sort (first `k` elements)
      distances.end(),       // End of the input range
      [](const std::pair<float, std::pair<int, std::string>> &a,
         const std::pair<float, std::pair<int, std::string>> &b)
      { return a.first < b.first; } // Custom comparison function (sort by the `first` element)
  );

  // Store the K nearest neighbours
  std::vector<std::string> labels(k);
  for (int i = 0; i < k; i++)
  {
    labels[i] = distances[i].second.second;
  }

  // Find the most common label
  std::string mostCommonLabel = labels[0];
  int maxCount = 1;
  for (int i = 0; i < k; i++)
  {
    int count = 1;
    for (int j = i + 1; j < k; j++)
    {
      if (labels[i] == labels[j])
      {
        count++;
      }
    }
    if (count > maxCount)
    {
      maxCount = count;
      mostCommonLabel = labels[i];
    }
  }

  // Ensure a confidence of atleast 80%
  float confidence = static_cast<float>(maxCount) / k;

  if (confidence >= 0.8)
  {
    Serial.print("Predicted label: ");
    Serial.println(mostCommonLabel.c_str());
    Serial.print("Confidence: ");
    Serial.print(confidence * 100);
    Serial.println("%");

    Serial.println("Top labels with shortest distances:");
    for (int i = 0; i < k; i++)
    {
      Serial.print(i + 1);
      Serial.print(". Index: ");
      Serial.print(distances[i].second.first);
      Serial.print(", Label: ");
      Serial.print(distances[i].second.second.c_str());
      Serial.print(" (Distance: ");
      Serial.print(distances[i].first);
      Serial.println(")");
    }

    if (mostCommonLabel != " At rest")
    {
      JSONVar predic;
      predic["pred"] = mostCommonLabel.c_str();
      String prediction = JSON.stringify(predic);
      // Send Events to the Web Server with the Sensor Readings
      events.send(prediction.c_str(), "prediction", millis());
      mode = 0;
      gyroDelay = 50;
      resetQueues();
      counter = 0;
      delay(1000);
    }
  }
}

void loop()
{

  if (myQueueFlex1.isFull())
  {
    myQueueFlex1.dequeue();
  }
  if (myQueueFlex2.isFull())
  {
    myQueueFlex2.dequeue();
  }
  if (myQueueFlex3.isFull())
  {
    myQueueFlex3.dequeue();
  }
  if (myQueueFlex4.isFull())
  {
    myQueueFlex4.dequeue();
  }
  if (myQueueFlex5.isFull())
  {
    myQueueFlex5.dequeue();
  }
  if (myQueueGyrox.isFull())
  {
    myQueueGyrox.dequeue();
  }
  if (myQueueGyroy.isFull())
  {
    myQueueGyroy.dequeue();
  }
  if (myQueueGyroz.isFull())
  {
    myQueueGyroz.dequeue();
  }
  if (myQueueAccelx.isFull())
  {
    myQueueAccelx.dequeue();
  }
  if (myQueueAccely.isFull())
  {
    myQueueAccely.dequeue();
  }
  if (myQueueAccelz.isFull())
  {
    myQueueAccelz.dequeue();
  }
  JSONVar readings;
  // Gather readings from sensors and store in JSONVar
  flexorval = analogRead(flexorpin) * (1.25 / 2700.0);
  flexorval2 = analogRead(flexorpin2) * (1.25 / 3300.0);
  flexorval3 = analogRead(flexorpin3) * (1.25 / 3100.0);
  flexorval4 = analogRead(flexorpin4) * (1.25 / 4095.0);
  flexorval5 = analogRead(flexorpin5) * (1.25 / 4095.0);
  readings["thumb"] = String(flexorval5);
  readings["Index"] = String(flexorval4);
  readings["Middle"] = String(flexorval3);
  readings["Ring"] = String(flexorval2);
  readings["Pinky"] = String(flexorval);
  mpu.getEvent(&a, &g, &temp);
  float gyroX_temp = g.gyro.x;
  if (abs(gyroX_temp) > gyroXerror)
  {
    gyroX += gyroX_temp / 100.00;
  }
  float gyroY_temp = g.gyro.y;
  if (abs(gyroY_temp) > gyroYerror)
  {
    gyroY += gyroY_temp / 140.00;
  }
  float gyroZ_temp = g.gyro.z;
  if (abs(gyroZ_temp) > gyroZerror)
  {
    gyroZ += gyroZ_temp / 180.00;
  }
  accX = a.acceleration.x;
  accY = a.acceleration.y;
  accZ = a.acceleration.z;
  readings["accX"] = String(accX);
  readings["accY"] = String(accY);
  readings["accZ"] = String(accZ);

  readings["gyroX"] = String(gyroX);
  readings["gyroY"] = String(gyroY);
  readings["gyroZ"] = String(gyroZ);
  String jsonString = JSON.stringify(readings);

  if ((millis() - lastTime) > gyroDelay)
  {
    // Send Events to the Web Server with the Sensor Readings
    events.send(jsonString.c_str(), "flex_sensor", millis());
    lastTime = millis();

    myQueueFlex1.enqueue(flexorval);
    myQueueFlex2.enqueue(flexorval2);
    myQueueFlex3.enqueue(flexorval3);
    myQueueFlex4.enqueue(flexorval4);
    myQueueFlex5.enqueue(flexorval5);
    myQueueGyrox.enqueue(normalize(gyroX, GYRO_MIN, GYRO_MAX, FLEX_MIN, FLEX_MAX));
    myQueueGyroy.enqueue(normalize(gyroY, GYRO_MIN, GYRO_MAX, FLEX_MIN, FLEX_MAX));
    myQueueGyroz.enqueue(normalize(gyroZ, GYRO_MIN, GYRO_MAX, FLEX_MIN, FLEX_MAX));
    myQueueAccelx.enqueue(normalize(accX, ACCEL_MIN, ACCEL_MAX, FLEX_MIN, FLEX_MAX));
    myQueueAccely.enqueue(normalize(accY, ACCEL_MIN, ACCEL_MAX, FLEX_MIN, FLEX_MAX));
    myQueueAccelz.enqueue(normalize(accZ, ACCEL_MIN, ACCEL_MAX, FLEX_MIN, FLEX_MAX));
    // delay(250);
    DataCSV_ML dataInstance;

    // Store readings in array
    for (int i = 0; i < 11; i++)
    {
      for (int k = 0; k < 12; k++)
      {
        int index = k + (i * 12);
        if (i == 0 && k < myQueueFlex1.getCount())
        {
          dataInstance.Avals[index] = myQueueFlex1.getAt(k);
        }
        else if (i == 1 && k < myQueueFlex2.getCount())
        {
          dataInstance.Avals[index] = myQueueFlex2.getAt(k);
        }
        else if (i == 2 && k < myQueueFlex3.getCount())
        {
          dataInstance.Avals[index] = myQueueFlex3.getAt(k);
        }
        else if (i == 3 && k < myQueueFlex4.getCount())
        {
          dataInstance.Avals[index] = myQueueFlex4.getAt(k);
        }
        else if (i == 4 && k < myQueueFlex5.getCount())
        {
          dataInstance.Avals[index] = myQueueFlex5.getAt(k);
        }
        else if (i == 5 && k < myQueueGyrox.getCount())
        {
          dataInstance.Avals[index] = myQueueGyrox.getAt(k);
        }
        else if (i == 6 && k < myQueueGyroy.getCount())
        {
          dataInstance.Avals[index] = myQueueGyroy.getAt(k);
        }
        else if (i == 7 && k < myQueueGyroz.getCount())
        {
          dataInstance.Avals[index] = myQueueGyroz.getAt(k);
        }
        else if (i == 8 && k < myQueueAccelx.getCount())
        {
          dataInstance.Avals[index] = myQueueAccelx.getAt(k);
        }
        else if (i == 9 && k < myQueueAccely.getCount())
        {
          dataInstance.Avals[index] = myQueueAccely.getAt(k);
        }
        else if (i == 10 && k < myQueueAccelz.getCount())
        {
          dataInstance.Avals[index] = myQueueAccelz.getAt(k);
        }
      }
    }

    // Check if in running mode
    if (mode == 1)
    {

      if (counter == 0)
      {
        Serial.println("Start action");
        delay(1000);
      }
      counter++;
      if (counter == 11)
      {
        GetKNN(k, dataInstance);

        counter = 0;
      }
    }
    else if (mode == 0)
    {
      // Used for training
      /*
      // Assign a label to the data
      dataInstance.label = "Label";
      // Output the data
      if (counter == 0)
      {
           Serial.println("Start action");
         delay(1000);
      }
      counter++;
      if (counter == 11)
      {
        std::cout << "";
        for (int i = 0; i < 132; i++)
        {
           std::cout << dataInstance.Avals[i] << ", ";
        }
          std::cout << "" << dataInstance.label << std::endl;
        counter = 0;

      }
      */
    }
  }
}
