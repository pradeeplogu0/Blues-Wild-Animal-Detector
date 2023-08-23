#include <Arduino.h>
#include <Notecard.h>
#include <Wire.h>

#define PRODUCT_UID "com.gmail.pradeeplogu26:cold_storage_monitor"

#define myProductID PRODUCT_UID
Notecard notecard;

#include <Adafruit_NeoPixel.h>

int Power = 11;
int PIN  = 12;
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

static char recv_buf[512];
static bool is_exist = false;

static int at_send_check_response(char *p_ack, int timeout_ms, char *p_cmd, ...)
{
  int ch = 0;
  int index = 0;
  int startMillis = 0;
  va_list args;
  memset(recv_buf, 0, sizeof(recv_buf));
  va_start(args, p_cmd);
  Serial1.printf(p_cmd, args);
  Serial.printf(p_cmd, args);
  va_end(args);
  delay(200);
  startMillis = millis();

  if (p_ack == NULL)
  {
    return 0;
  }

  do
  {
    while (Serial1.available() > 0)
    {
      ch = Serial1.read();
      recv_buf[index++] = ch;
      Serial.print((char)ch);
      delay(2);
    }

    if (strstr(recv_buf, p_ack) != NULL)
    {
      return 1;
    }

  } while (millis() - startMillis < timeout_ms);
  return 0;
}

static int recv_prase(void)
{
  char ch;
  int index = 0;
  memset(recv_buf, 0, sizeof(recv_buf));
  while (Serial1.available() > 0)
  {
    ch = Serial1.read();
    recv_buf[index++] = ch;
    Serial.print((char)ch);
    delay(2);
  }

  if (index)
  {
    char *p_start = NULL;
    char data[32] = {
      0,
    };
    int rssi = 0;
    int snr = 0;

    p_start = strstr(recv_buf, "+TEST: RX \"5345454544");
    if (p_start)
    {
      p_start = strstr(recv_buf, "5345454544");
      if (p_start && (1 == sscanf(p_start, "5345454544%s,", data)))
      {
        data[16] = 0;
        int node;
        int alert;

        char *endptr;
        char *endptr1;
        char *endptr2;
        char *endptr3;
        char datatvoc[5] = {data[0], data[1], data[2], data[3]};
        char dataco2[5] = {data[4], data[5], data[6], data[7]};

        node = strtol(datatvoc, &endptr, 16);
        alert = strtol(dataco2, &endptr1, 16);


        double temperature = 0;
        J *rsp = notecard.requestAndResponse(notecard.newRequest("card.temp"));
        if (rsp != NULL) {
          temperature = JGetNumber(rsp, "value");
          notecard.deleteResponse(rsp);
        }

        double voltage = 0;
        rsp = notecard.requestAndResponse(notecard.newRequest("card.voltage"));
        if (rsp != NULL) {
          voltage = JGetNumber(rsp, "value");
          notecard.deleteResponse(rsp);
        }

        J *req = notecard.newRequest("note.add");
        if (req != NULL) {
          JAddBoolToObject(req, "sync", true);
          J *body = JCreateObject();
          if (body != NULL) {
            JAddNumberToObject(body, "Node", node);
            JAddNumberToObject(body, "Alert", alert);
          }
          notecard.sendRequest(req);
          Serial.println("NoteCard Data Sent");
        }
      }

      p_start = strstr(recv_buf, "RSSI:");
      if (p_start && (1 == sscanf(p_start, "RSSI:%d,", &rssi)))
      {
        String newrssi = String(rssi);

        Serial.print(rssi);
        Serial.print("\r\n");

      }
      p_start = strstr(recv_buf, "SNR:");
      if (p_start && (1 == sscanf(p_start, "SNR:%d", &snr)))
      {
        Serial.print(snr);
        Serial.print("\r\n");


      }
      return 1;
    }
  }
  return 0;
}

static int node_recv(uint32_t timeout_ms)
{
  at_send_check_response("+TEST: RXLRPKT", 1000, "AT+TEST=RXLRPKT\r\n");
  int startMillis = millis();
  do
  {
    if (recv_prase())
    {
      return 1;
    }
  } while (millis() - startMillis < timeout_ms);
  return 0;
}

void setup()
{

  Wire.begin();
  notecard.begin();

  J *req = notecard.newRequest("hub.set");
  if (myProductID[0]) {
    JAddStringToObject(req, "product", myProductID);
  }
  JAddStringToObject(req, "mode", "continuous");

  notecard.sendRequest(req);


  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.print("Receiver\r\n");

  if (at_send_check_response("+AT: OK", 100, "AT\r\n"))
  {
    is_exist = true;
    at_send_check_response("+MODE: TEST", 1000, "AT+MODE=TEST\r\n");
    at_send_check_response("+TEST: RFCFG", 1000, "AT+TEST=RFCFG,868,SF12,125,12,15,14,ON,OFF,OFF\r\n");
    delay(200);
  }
  else
  {
    is_exist = false;
    Serial.print("No Serial1 module found.\r\n");

  }

  pixels.begin();
  pinMode(Power, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(Power, HIGH);
  delay(200);
}


void loop()
{
  if (is_exist)
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(0, 5, 0));
    pixels.show();
    delay(500);

    node_recv(2000);

    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(5, 0, 0));
    pixels.show();
    delay(500);

  }

}
