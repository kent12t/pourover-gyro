// Include this to enable the M5 global instance.
// get via arduino lib
#include <M5Unified.h>  // by m5stack

// get via arduino lib
#include <ArduinoJson.h>  // by benoit blanchon

// Data structure for IMU data
struct IMUData {
  // float ax, ay, az, gx, gy, gz, mx, my, mz, q0, q1, q2, q3, roll, pitch, heading;
  // float ax, ay, az, rxy, roll, pitch, heading;
  float rxy, pitch;
  unsigned long timestamp;
};

const int maxDataPoints = 20;  // Adjust based on expected data during interval
IMUData dataStorage[maxDataPoints];
int dataCount = 0;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100;  // Send data every 100ms

// madgwick lib
// updated in the esp32 directory
#include <MadgwickAHRS.h>  // adapted from arduino

#include <cstring>
#include <float.h>
#include <math.h>

// Constants
const int SAMPLE_SIZE = 100;  // Number of samples for recent average

// Variables
float radialAccels[SAMPLE_SIZE] = { 0 };  // Array to store radial accelerations
int sampleCount = 0;                      // Total number of samples received
float totalSum = 0;                       // Sum of all radial accelerations for calculating total average

// esp32 connectivity
#include <WiFi.h>
// get via arduino lib
#include <ESPAsyncWebServer.h>  // by lacamera
#include <AsyncTCP.h>           // dependency from above lib

AsyncWebServer server(80);
AsyncEventSource events("/events");  // SSE endpoint

const char* ssid = "Nothing";
const char* password = "kent1234";
// 192.168.116.39
// STA IP: 192.168.116.39, MASK: 255.255.255.0, GW: 192.168.116.106

// additional state management
bool wifiConnected = false;
bool serverLive = false;

unsigned long lastUpdateTime = 0;  // Stores the time of the last IMU update
unsigned long currentTime = 0;     // Stores the current time at each loop

// Access imu and quaternion components
float q0;
float q1;
float q2;
float q3;
float roll;
float pitch;
float heading;
float ax;
float ay;
float az;
float gx;
float gy;
float gz;
float mx;
float my;
float mz;
// float recentAverage;
float rxy;

// Global variables to hold the max and min sensor values
float maxAccelX = -FLT_MAX, minAccelX = FLT_MAX;  // -5.36 || 5.05
float maxAccelY = -FLT_MAX, minAccelY = FLT_MAX;  // -7.999 || 4.86
float maxAccelZ = -FLT_MAX, minAccelZ = FLT_MAX;  // -6.48 || 7.70
float maxGyroX = -FLT_MAX, minGyroX = FLT_MAX;    // -1999 || 1677
float maxGyroY = -FLT_MAX, minGyroY = FLT_MAX;    // -1600 || 1508
float maxGyroZ = -FLT_MAX, minGyroZ = FLT_MAX;    // -1999 || 1999
float maxMagX = -FLT_MAX, minMagX = FLT_MAX;      // -646 || 85
float maxMagY = -FLT_MAX, minMagY = FLT_MAX;      // -581 || 174
float maxMagZ = -FLT_MAX, minMagZ = FLT_MAX;      // -357 || 356

// Strength of the calibration operation;
// 0: disables calibration.
// 1 is weakest and 255 is strongest.
static constexpr const uint8_t calib_value = 64;

// This sample code performs calibration by clicking on a button or screen.
// After 10 seconds of calibration, the results are stored in NVS.
// The saved calibration values are loaded at the next startup.
//
// === How to calibration ===
// ※ Calibration method for Accelerometer
//    Change the direction of the main unit by 90 degrees
//     and hold it still for 2 seconds. Repeat multiple times.
//     It is recommended that as many surfaces as possible be on the bottom.
//
// ※ Calibration method for Gyro
//    Simply place the unit on a quiet desk and hold it still.
//    It is recommended that this be done after the accelerometer calibration.
//
// ※ Calibration method for geomagnetic sensors
//    Rotate the main unit slowly in multiple directions.
//    It is recommended that as many surfaces as possible be oriented to the north.
//
// Values for extremely large attitude changes are ignored.
// During calibration, it is desirable to move the device as gently as possible.

struct rect_t {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
};

static constexpr const uint32_t color_tbl[18] = {
  0xFF0000u,
  0xCCCC00u,
  0xCC00FFu,
  0xFFCC00u,
  0x00FF00u,
  0x0088FFu,
  0xFF00CCu,
  0x00FFCCu,
  0x0000FFu,
  0xFF0000u,
  0xCCCC00u,
  0xCC00FFu,
  0xFFCC00u,
  0x00FF00u,
  0x0088FFu,
  0xFF00CCu,
  0x00FFCCu,
  0x0000FFu,
};
static constexpr const float coefficient_tbl[3] = { 0.5f, (1.0f / 256.0f), (1.0f / 1024.0f) };

static auto& dsp = (M5.Display);
static rect_t rect_graph_area;
static rect_t rect_text_area;

static uint8_t calib_countdown = 0;

static int prev_xpos[18];

Madgwick filter;
float sampleFreq = 100.0;  // Sample frequency in Hz

float lastAccelX = 0;
float lastAccelY = 0;


void drawBar(int32_t ox, int32_t oy, int32_t nx, int32_t px, int32_t h, uint32_t color) {
  uint32_t bgcolor = (color >> 3) & 0x1F1F1Fu;
  if (px && ((nx < 0) != (px < 0))) {
    dsp.fillRect(ox, oy, px, h, bgcolor);
    px = 0;
  }
  if (px != nx) {
    if ((nx > px) != (nx < 0)) {
      bgcolor = color;
    }
    dsp.setColor(bgcolor);
    dsp.fillRect(nx + ox, oy, px - nx, h);
  }
}

void drawGraph(const rect_t& r, const m5::imu_data_t& data) {
  float aw = (128 * r.w) >> 1;
  float gw = (128 * r.w) / 256.0f;
  float mw = (128 * r.w) / 1024.0f;
  int ox = (r.x + r.w) >> 1;
  int oy = r.y;
  int h = (r.h / 18) * (calib_countdown ? 1 : 2);
  int bar_count = 9 * (calib_countdown ? 2 : 1);

  dsp.startWrite();
  for (int index = 0; index < bar_count; ++index) {
    float xval;
    if (index < 9) {
      auto coe = coefficient_tbl[index / 3] * r.w;
      xval = data.value[index] * coe;
    } else {
      xval = M5.Imu.getOffsetData(index - 9) * (1.0f / (1 << 19));
    }

    // for Linear scale graph.
    float tmp = xval;

    // The smaller the value, the larger the amount of change in the graph.
    //  float tmp = sqrtf(fabsf(xval * 128)) * (signbit(xval) ? -1 : 1);

    int nx = tmp;
    int px = prev_xpos[index];
    if (nx != px)
      prev_xpos[index] = nx;
    drawBar(ox, oy + h * index, nx, px, h - 1, color_tbl[index]);
  }
  dsp.endWrite();
}

void updateCalibration(uint32_t c, bool clear = false) {
  calib_countdown = c;

  if (c == 0) {
    clear = true;
  }

  if (clear) {
    memset(prev_xpos, 0, sizeof(prev_xpos));
    dsp.fillScreen(TFT_BLACK);

    if (c) {  // Start calibration.
      M5.Imu.setCalibration(calib_value, calib_value, calib_value);
      // ※ The actual calibration operation is performed each time during M5.Imu.update.
      //
      // There are three arguments, which can be specified in the order of Accelerometer, gyro, and geomagnetic.
      // If you want to calibrate only the Accelerometer, do the following.
      // M5.Imu.setCalibration(100, 0, 0);
      //
      // If you want to calibrate only the gyro, do the following.
      // M5.Imu.setCalibration(0, 100, 0);
      //
      // If you want to calibrate only the geomagnetism, do the following.
      // M5.Imu.setCalibration(0, 0, 100);
    } else {  // Stop calibration. (Continue calibration only for the geomagnetic sensor)
      M5.Imu.setCalibration(0, 0, calib_value);

      // If you want to stop all calibration, write this.
      // M5.Imu.setCalibration(0, 0, 0);

      // save calibration values.
      M5.Imu.saveOffsetToNVS();
    }
  }

  auto backcolor = (c == 0) ? TFT_BLACK : TFT_BLUE;
  dsp.fillRect(rect_text_area.x, rect_text_area.y, rect_text_area.w, rect_text_area.h, backcolor);

  if (c) {
    dsp.setCursor(rect_text_area.x + 2, rect_text_area.y + 1);
    dsp.setTextColor(TFT_WHITE, TFT_BLUE);
    dsp.printf("Countdown:%d ", c);
  }
}

void startCalibration(void) {
  updateCalibration(10, true);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
}

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

/*//////////////////
<<<<<< SETUP >>>>>>>
//////////////////*/


void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(921600);
  filter.begin(sampleFreq);

  connectToWiFi();

  // Setup EventSource onConnect
  events.onConnect([](AsyncEventSourceClient* client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // Send an initial hello message on connect
    client->send("hello!", "message", millis(), 1000);
  });

  // handle annoying CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  // Add SSE endpoint to the server
  server.addHandler(&events);
  server.onNotFound(notFound);
  server.begin();

  int32_t w = dsp.width();
  int32_t h = dsp.height();
  if (w < h) {
    dsp.setRotation(dsp.getRotation() ^ 1);
    w = dsp.width();
    h = dsp.height();
  }
  int32_t graph_area_h = ((h - 8) / 18) * 18;
  int32_t text_area_h = h - graph_area_h;
  float fontsize = text_area_h / 8;
  dsp.setTextSize(fontsize);

  rect_graph_area = { 0, 0, w, graph_area_h };
  rect_text_area = { 0, graph_area_h, w, text_area_h };


  // Read calibration values from NVS.
  if (!M5.Imu.loadOffsetFromNVS()) {
    startCalibration();
  }
}

/*/////////////////
<<<<<< LOOP >>>>>>>
/////////////////*/

void loop(void) {
  static uint32_t frame_count = 0;
  static uint32_t prev_sec = 0;

  currentTime = millis();

  // check wifi
  if (WiFi.status() == WL_CONNECTED && wifiConnected == false) {
    wifiConnected = true;
    IPAddress ip = WiFi.localIP();
    M5_LOGI("IP address: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  }

  // To update the IMU value, use M5.Imu.update.
  // If a new value is obtained, the return value is non-zero.
  auto imu_update = M5.Imu.update();

  if (imu_update) {
    if (lastUpdateTime != 0) {  // Ensure this is not the first update
      int elapsedTime = currentTime - lastUpdateTime;
      // M5_LOGV("%i", elapsedTime);
    }
    lastUpdateTime = currentTime;  // Update the last update time


    // Obtain data on the current value of the IMU.
    auto data = M5.Imu.getImuData();
    // auto data = M5.Imu.getImuData();
    drawGraph(rect_graph_area, data);

    // The data obtained by getImuData can be used as follows.
    ax = data.accel.x;  // accel x-axis value.
    ay = data.accel.y;  // accel y-axis value.
    az = data.accel.z;  // accel z-axis value.
    // data.accel.value;  // accel 3values array [0]=x / [1]=y / [2]=z.

    gx = data.gyro.x;  // gyro x-axis value.
    gy = data.gyro.y;  // gyro y-axis value.
    gz = data.gyro.z;  // gyro z-axis value.
    // data.gyro.value;  // gyro 3values array [0]=x / [1]=y / [2]=z.

    mx = data.mag.x;  // mag x-axis value.
    my = data.mag.y;  // mag y-axis value.
    mz = data.mag.z;  // mag z-axis value.
    // data.mag.value;   // mag 3values array [0]=x / [1]=y / [2]=z.

    // data.value;       // all sensor 9values array [0~2]=accel / [3~5]=gyro / [6~8]=mag

    M5_LOGV("ax:%f  ay:%f  az:%f", data.accel.x, data.accel.y, data.accel.z);
    // M5_LOGV("gx:%f  gy:%f  gz:%f", data.gyro.x , data.gyro.y , data.gyro.z );
    // M5_LOGV("mx:%f  my:%f  mz:%f", data.mag.x  , data.mag.y  , data.mag.z  );

    filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

    // Access quaternion components
    q0 = filter.getQ0();
    q1 = filter.getQ1();
    q2 = filter.getQ2();
    q3 = filter.getQ3();
    // M5_LOGV("q0:%f  q1:%f  q2:%f  q3:%f", q0, q1, q2, q3);

    // print the heading, pitch and roll
    roll = filter.getRoll();
    pitch = filter.getPitch();
    heading = filter.getYaw();
    // M5_LOGV("r:%f  p:%f  h:%f", roll, pitch, heading);

    // Calculate current radial acceleration
    float currentRadialAccel = sqrt(ax * ax + ay * ay);
    // Update total sum and sample count for total average calculation
    totalSum += currentRadialAccel;
    sampleCount++;

    // Store current radial acceleration in the array and shift old data
    for (int i = SAMPLE_SIZE - 1; i > 0; i--) {
      radialAccels[i] = radialAccels[i - 1];  // Shift data in the array
    }
    radialAccels[0] = currentRadialAccel;  // Add the newest sample at the start of the array

    // Calculate the total average
    float totalAverage = totalSum / sampleCount;

    // Calculate the recent average from the array
    float recentSum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
      recentSum += radialAccels[i];
    }
    float recentAverage = recentSum / SAMPLE_SIZE;

    rxy = recentAverage;


    // M5_LOGV("%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", (data.gyro.x, data.gyro.y, data.gyro.z, data.accel.x, data.accel.y, data.accel.z, data.mag.x, data.mag.y, data.mag.z, q0, q1, q2, q3, roll, pitch, heading));

    if (dataCount < maxDataPoints) {

      IMUData& data = dataStorage[dataCount++];
      data.timestamp = currentTime;
      // Populate data fields here
      // data.ax = roundf(ax * 1000) / 1000.0;
      // data.ay = roundf(ay * 1000) / 1000.0;
      // data.az = roundf(az * 1000) / 1000.0;
      // data.gx = roundf(gx * 1000) / 1000.0;
      // data.gy = roundf(gy * 1000) / 1000.0;
      // data.gz = roundf(gz * 1000) / 1000.0;
      // data.mx = roundf(mx * 1000) / 1000.0;
      // data.my = roundf(my * 1000) / 1000.0;
      // data.mz = roundf(mz * 1000) / 1000.0;
      // data.q0 = roundf(q0 * 1000) / 1000.0;
      // data.q1 = roundf(q1 * 1000) / 1000.0;
      // data.q2 = roundf(q2 * 1000) / 1000.0;
      // data.q3 = roundf(q3 * 1000) / 1000.0;

      data.rxy = rxy;
      // data.roll = roundf(roll * 1000) / 1000.0;
      data.pitch = pitch;
      // data.heading = roundf(heading * 1000) / 1000.0;
    }

    ++frame_count;
  } else {
    M5.update();

    // Calibration is initiated when a button or screen is clicked.
    if (M5.BtnA.wasClicked() || M5.BtnPWR.wasClicked() || M5.Touch.getDetail().wasClicked()) {
      startCalibration();
    }
  }


  // Check if it's time to send the data
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;

    // Prepare JSON array for sending
    size_t jsonCapacity = sizeof(IMUData) * dataCount * 10;
    DynamicJsonDocument doc(jsonCapacity);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < dataCount; i++) {
      JsonObject obj = array.createNestedObject();
      obj["timestamp"] = dataStorage[i].timestamp;
      // obj["ax"] = dataStorage[i].ax;
      // obj["ay"] = dataStorage[i].ay;
      // obj["az"] = dataStorage[i].az;
      obj["rxy"] = dataStorage[i].rxy;


      // obj["gx"] = dataStorage[i].gx;
      // obj["gy"] = dataStorage[i].gy;
      // obj["gz"] = dataStorage[i].gz;
      // obj["mx"] = dataStorage[i].mx;
      // obj["my"] = dataStorage[i].my;
      // obj["mz"] = dataStorage[i].mz;
      // obj["q0"] = dataStorage[i].q0;
      // obj["q1"] = dataStorage[i].q1;
      // obj["q2"] = dataStorage[i].q2;
      // obj["q3"] = dataStorage[i].q3;
      // obj["roll"] = dataStorage[i].roll;
      obj["pitch"] = dataStorage[i].pitch;
      // obj["heading"] = dataStorage[i].heading;
      // Add other fields similarly
    }

    // Serialize JSON and send
    char buffer[1024*5];
    serializeJson(doc, buffer, sizeof(buffer));
    events.send(buffer, "imu_data", millis());



    // Reset data count for new data
    dataCount = 0;
  }


  int32_t sec = millis() / 1000;
  if (prev_sec != sec) {
    prev_sec = sec;
    // M5_LOGI("sec:%d  frame:%d", sec, frame_count);
    frame_count = 0;

    if (calib_countdown) {
      updateCalibration(calib_countdown - 1);
    }

    if ((sec & 7) == 0) {  // prevent WDT.
      vTaskDelay(1);
    }
  }
}
