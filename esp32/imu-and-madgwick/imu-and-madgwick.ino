// Include this to enable the M5 global instance.
// downloadable via arduino lib
#include <M5Unified.h>
// madgwick lib included in the esp32 folder
#include <MadgwickAHRS.h>

#include <cstring>
#include <float.h>

#include <WiFi.h>

// downloadable via arduino lib or
// https://github.com/gilmaimon/ArduinoWebsockets
#include <ArduinoWebsockets.h>

const char *ssid = "Nothing";
const char *password = "kent1234";

bool wifiConnected = false;
bool serverLive = false;

// 192.168.116.39
// STA IP: 192.168.116.39, MASK: 255.255.255.0, GW: 192.168.116.106

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 1000; // Attempt to reconnect every 1 second

unsigned long lastUpdateTime = 0; // Stores the time of the last IMU update
unsigned long currentTime = 0;    // Stores the current time at each loop

using namespace websockets;

WebsocketsServer server;

// Access quaternion components
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

// Global variables to hold the max and min sensor values
float maxAccelX = -FLT_MAX, minAccelX = FLT_MAX; // -5.36 || 5.05
float maxAccelY = -FLT_MAX, minAccelY = FLT_MAX; // -7.999 || 4.86
float maxAccelZ = -FLT_MAX, minAccelZ = FLT_MAX; // -6.48 || 7.70
float maxGyroX = -FLT_MAX, minGyroX = FLT_MAX;   // -1999 || 1677
float maxGyroY = -FLT_MAX, minGyroY = FLT_MAX;   // -1600 || 1508
float maxGyroZ = -FLT_MAX, minGyroZ = FLT_MAX;   // -1999 || 1999
float maxMagX = -FLT_MAX, minMagX = FLT_MAX;     // -646 || 85
float maxMagY = -FLT_MAX, minMagY = FLT_MAX;     // -581 || 174
float maxMagZ = -FLT_MAX, minMagZ = FLT_MAX;     // -357 || 356

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

struct rect_t
{
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
static constexpr const float coefficient_tbl[3] = {0.5f, (1.0f / 256.0f), (1.0f / 1024.0f)};

static auto &dsp = (M5.Display);
static rect_t rect_graph_area;
static rect_t rect_text_area;

static uint8_t calib_countdown = 0;

static int prev_xpos[18];

Madgwick filter;
float sampleFreq = 100.0; // Sample frequency in Hz

float lastAccelX = 0;
float lastAccelY = 0;

void drawBar(int32_t ox, int32_t oy, int32_t nx, int32_t px, int32_t h, uint32_t color)
{
  uint32_t bgcolor = (color >> 3) & 0x1F1F1Fu;
  if (px && ((nx < 0) != (px < 0)))
  {
    dsp.fillRect(ox, oy, px, h, bgcolor);
    px = 0;
  }
  if (px != nx)
  {
    if ((nx > px) != (nx < 0))
    {
      bgcolor = color;
    }
    dsp.setColor(bgcolor);
    dsp.fillRect(nx + ox, oy, px - nx, h);
  }
}

void drawGraph(const rect_t &r, const m5::imu_data_t &data)
{
  float aw = (128 * r.w) >> 1;
  float gw = (128 * r.w) / 256.0f;
  float mw = (128 * r.w) / 1024.0f;
  int ox = (r.x + r.w) >> 1;
  int oy = r.y;
  int h = (r.h / 18) * (calib_countdown ? 1 : 2);
  int bar_count = 9 * (calib_countdown ? 2 : 1);

  dsp.startWrite();
  for (int index = 0; index < bar_count; ++index)
  {
    float xval;
    if (index < 9)
    {
      auto coe = coefficient_tbl[index / 3] * r.w;
      xval = data.value[index] * coe;
    }
    else
    {
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

void updateCalibration(uint32_t c, bool clear = false)
{
  calib_countdown = c;

  if (c == 0)
  {
    clear = true;
  }

  if (clear)
  {
    memset(prev_xpos, 0, sizeof(prev_xpos));
    dsp.fillScreen(TFT_BLACK);

    if (c)
    { // Start calibration.
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
    }
    else
    { // Stop calibration. (Continue calibration only for the geomagnetic sensor)
      M5.Imu.setCalibration(0, 0, calib_value);

      // If you want to stop all calibration, write this.
      // M5.Imu.setCalibration(0, 0, 0);

      // save calibration values.
      M5.Imu.saveOffsetToNVS();
    }
  }

  auto backcolor = (c == 0) ? TFT_BLACK : TFT_BLUE;
  dsp.fillRect(rect_text_area.x, rect_text_area.y, rect_text_area.w, rect_text_area.h, backcolor);

  if (c)
  {
    dsp.setCursor(rect_text_area.x + 2, rect_text_area.y + 1);
    dsp.setTextColor(TFT_WHITE, TFT_BLUE);
    dsp.printf("Countdown:%d ", c);
  }
}

void startCalibration(void)
{
  updateCalibration(10, true);
}

void connectToWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - lastReconnectAttempt >= reconnectInterval)
    {
      lastReconnectAttempt = currentMillis;
      M5_LOGI("Attempting to connect to WiFi...");

      WiFi.begin(ssid, password);
      delay(100);
    }
  }
  else
  {
    // Serial.print("Connected to WiFi, IP address: ");
    // Serial.println(WiFi.localIP());
  }
}

void sendSensorData(WebsocketsClient &client)
{
  // Ensure your sensor data structure is globally accessible or pass it here
  // char data[300];  // Ensure this buffer is large enough for your data string
  // snprintf(data, sizeof(data), "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
  //          gx, gy, gz,
  //          ax, ay, az,
  //          mx, my, mz,
  //          q0, q1, q2, q3,
  //          roll, pitch, heading);

  // // Send the formatted sensor data to the client
  // client.send(data);

  float data[16] = {ax, ay, az, gx, gy, gz, mx, my, mz, q0, q1, q2, q3, roll, pitch, heading};
  uint8_t buffer[sizeof(data)];
  memcpy(buffer, data, sizeof(data));
  // Cast buffer to const char* when passing to sendBinary
  client.sendBinary((const char *)buffer, sizeof(buffer));
}

/*//////////////////
<<<<<< SETUP >>>>>>>
//////////////////*/

void setup(void)
{
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(921600);
  filter.begin(sampleFreq);

  connectToWiFi();

  int32_t w = dsp.width();
  int32_t h = dsp.height();
  if (w < h)
  {
    dsp.setRotation(dsp.getRotation() ^ 1);
    w = dsp.width();
    h = dsp.height();
  }
  int32_t graph_area_h = ((h - 8) / 18) * 18;
  int32_t text_area_h = h - graph_area_h;
  float fontsize = text_area_h / 8;
  dsp.setTextSize(fontsize);

  rect_graph_area = {0, 0, w, graph_area_h};
  rect_text_area = {0, graph_area_h, w, text_area_h};

  // Read calibration values from NVS.
  if (!M5.Imu.loadOffsetFromNVS())
  {
    startCalibration();
  }
}

/*/////////////////
<<<<<< LOOP >>>>>>>
/////////////////*/

void loop(void)
{
  static uint32_t frame_count = 0;
  static uint32_t prev_sec = 0;

  currentTime = millis();

  connectToWiFi();

  if (WiFi.status() == WL_CONNECTED && wifiConnected == false)
  {
    wifiConnected = true;
    IPAddress ip = WiFi.localIP();
    M5_LOGI("IP address: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  }

  if (wifiConnected == true)
  {
    if (server.available() == false)
    {
      // if server still unavail, attempt to start
      server.listen(80);
      M5_LOGI("Restart Server Init");
      delay(100);
    }
    else if (serverLive == false)
    {
      serverLive = true;
      M5_LOGI("Server is Live");
    }
  }

  WebsocketsClient client;

  if (serverLive == true)
  {

    // update websocket
    client = server.accept();
  }

  // To update the IMU value, use M5.Imu.update.
  // If a new value is obtained, the return value is non-zero.
  auto imu_update = M5.Imu.update();

  if (imu_update)
  {
    if (lastUpdateTime != 0)
    { // Ensure this is not the first update
      int elapsedTime = currentTime - lastUpdateTime;
      // M5_LOGV("%i", elapsedTime);
    }
    lastUpdateTime = currentTime; // Update the last update time

    // Obtain data on the current value of the IMU.
    auto data = M5.Imu.getImuData();
    // auto data = M5.Imu.getImuData();
    drawGraph(rect_graph_area, data);

    // The data obtained by getImuData can be used as follows.
    ax = data.accel.x; // accel x-axis value.
    ay = data.accel.y; // accel y-axis value.
    az = data.accel.z; // accel z-axis value.
    // data.accel.value;  // accel 3values array [0]=x / [1]=y / [2]=z.

    gx = data.gyro.x; // gyro x-axis value.
    gy = data.gyro.y; // gyro y-axis value.
    gz = data.gyro.z; // gyro z-axis value.
    // data.gyro.value;  // gyro 3values array [0]=x / [1]=y / [2]=z.

    mx = data.mag.x; // mag x-axis value.
    my = data.mag.y; // mag y-axis value.
    mz = data.mag.z; // mag z-axis value.
    // data.mag.value;   // mag 3values array [0]=x / [1]=y / [2]=z.

    // data.value;       // all sensor 9values array [0~2]=accel / [3~5]=gyro / [6~8]=mag

    // M5_LOGV("ax:%f  ay:%f  az:%f", data.accel.x, data.accel.y, data.accel.z);
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

    // M5_LOGV("%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", (data.gyro.x, data.gyro.y, data.gyro.z, data.accel.x, data.accel.y, data.accel.z, data.mag.x, data.mag.y, data.mag.z, q0, q1, q2, q3, roll, pitch, heading));

    if (serverLive == true)
    {
      if (client.available())
      {
        // WebsocketsMessage msg = client.readBlocking();
        // log
        // Serial.print("Got Message: ");
        // Serial.println(msg.data());

        sendSensorData(client);
      }
    }

    ++frame_count;
  }
  else
  {
    M5.update();

    // Calibration is initiated when a button or screen is clicked.
    if (M5.BtnA.wasClicked() || M5.BtnPWR.wasClicked() || M5.Touch.getDetail().wasClicked())
    {
      startCalibration();
    }
  }

  int32_t sec = millis() / 1000;
  if (prev_sec != sec)
  {
    prev_sec = sec;
    // M5_LOGI("sec:%d  frame:%d", sec, frame_count);
    frame_count = 0;

    if (calib_countdown)
    {
      updateCalibration(calib_countdown - 1);
    }

    if ((sec & 7) == 0)
    { // prevent WDT.
      vTaskDelay(1);
    }
  }
}
