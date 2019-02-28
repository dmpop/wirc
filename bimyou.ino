// Author: Dmitri Popov, dmpop@linux.com
// License: GPLv3 https://www.gnu.org/licenses/gpl-3.0.txt
// Source code: https://gitlab.com/dmpop/bimyou

// This project is largely based on
// https://www.youtube.com/watch?v=D40ZpVjZ744

#include <ESP8266WiFi.h>
// Some Declarations
int IR_LED = 4;                // IR Output Signal to Anode of IR LED
// Cathode of IR LED to ground through a 150 Ohm Resistor.
int BEGIN = 0 ;               // Indicates Shutter Start on LED
#define LED     D0        // LED in NodeMCU at pin GPIO16 (D0)

int shutter[] = {1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int twosec[] = {1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int start[] = {0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int disp[] = {0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int hist[] = {1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int zin[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int zout[] = {1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int playback[] = {0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int slide[] = {1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int menu[] = {0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int up[] = {0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int trash[] = {1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int right[] = {1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int CodeBits[] = {1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int left[] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int down[] = {1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int prin[] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int inde[] = {0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int rotate[] = {0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
int enter[] = {1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1};

IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);


// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
// Routine to send header data burst
// This allows the IR reciever to set its AGC (Gain)
// Header Burst Timing is 96 * 0.025uS = 2.4mS
// Quiet Timing is 24 * 0.025uS = 600uS

// Routine to give the 40kHz burst signal
void burst()                   // 40KHz burst
{
  digitalWrite(IR_LED, HIGH);   // sets the pin on
  delayMicroseconds(10);       // pauses for 13 microseconds  (fudged to 10uS Delay)
  digitalWrite(IR_LED, LOW);    // sets the pin off
  delayMicroseconds(8);        // pauses for 12 microseconds   (fudged to 8uS Delay)
}
// Routine to give a quiet period of data
void quiet()                   // Quiet burst
{
  digitalWrite(IR_LED, LOW);    // sets the pin off
  delayMicroseconds(10);       // pauses for 13 microseconds   (fudged to 10uS Delay)
  digitalWrite(IR_LED, LOW);    // sets the pin off
  delayMicroseconds(8);        // pauses for 12 microseconds    (fudged to 8uS Delay)
}


void header()
{
  for (int i = 1; i <= 96; i++) {
    burst();                // 40kHz burst
  }
  for (int i = 1; i <= 24; i++) {
    quiet();                // No 40 kHz
  }
}

// Routine to send one data burst
// Burst Timing is 48 * 0.025uS = 1.2mS
// Quiet Timing is 24 * 0.025uS = 600uS
void Data_is_One()
{
  for (int i = 1; i <= 48; i++) {
    burst();                // 40kHz burst
  }
  for (int i = 1; i <= 24; i++) {
    quiet();                // No 40 kHz
  }
}

// Routine to send zero data burst
// Burst Timing is 24 * 0.025uS = 600uS
// Quiet Timing is 24 * 0.025uS = 600uS
void Data_is_Zero()
{
  for (int i = 1; i <= 24; i++) {
    burst();                // 40 kHz burst
  }
  for (int i = 1; i <= 24; i++) {
    quiet();                // No 40 kHz
  }
}

void sendbutton(int CodeBits[])
{

  digitalWrite(BEGIN, HIGH);  // Signal BEGIN LED ON

  for (int i = 1; i <= 3; i++) // Send Command 3 times as per Sony Specs
  {
    header();                    // Send the Start header
    for (int i = 0; i <= 19; i++) // Loop to send the bits
    {
      if (CodeBits[i] == 1) // Is Data_is_One to be sent ?
      {
        Data_is_One();              // Yes, send a Data_is_One bit
      }
      else                  // No, Data_is_Zero to be sent
      {
        Data_is_Zero();              // Send a Data_is_Zero bit
      }
    }
    delay(11);                  // Delay Padding to give approx 45mS between command starts
  }
  digitalWrite(BEGIN, LOW);     // BEGIN LED OFF
}



void setup() {
  pinMode(LED, OUTPUT);        // Initialize the LED pin as an output
  Serial.begin(115200);
  delay(10);
Serial.begin(115200);
  Serial.println();
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("ESPsoftAP_01") ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED, HIGH);    // Turn the LED off
  }
  Serial.println("");
  Serial.println("WiFi connected");
  digitalWrite(LED, LOW);    // Turn the LED on

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  // Setup IR_LED and BEGIN LED Pins as outputs
  pinMode(IR_LED, OUTPUT);      // sets the digital pin as output
  pinMode(BEGIN, OUTPUT);      // sets the digital pin as output
}

// Infinite Loop
void loop()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  if (req.indexOf("/gpio/shutter") != -1)
    sendbutton(shutter);
  else if (req.indexOf("/gpio/2sec") != -1)
    sendbutton(twosec);
  else if (req.indexOf("/gpio/start") != -1)
    sendbutton(start);
  else if (req.indexOf("/gpio/disp") != -1)
    sendbutton(disp);
  else if (req.indexOf("/gpio/hist") != -1)
    sendbutton(hist);
  else if (req.indexOf("/gpio/zin") != -1)
    sendbutton(zin);
  else if (req.indexOf("/gpio/zout") != -1)
    sendbutton(zout);
  else if (req.indexOf("/gpio/playback") != -1)
    sendbutton(playback);
  else if (req.indexOf("/gpio/slide") != -1)
    sendbutton(slide);
  else if (req.indexOf("/gpio/menu") != -1)
    sendbutton(menu);
  else if (req.indexOf("/gpio/up") != -1)
    sendbutton(up);
  else if (req.indexOf("/gpio/trash") != -1)
    sendbutton(trash);
  else if (req.indexOf("/gpio/right") != -1)
    sendbutton(right);
  else if (req.indexOf("/gpio/enter") != -1)
    sendbutton(enter);
  else if (req.indexOf("/gpio/left") != -1)
    sendbutton(left);
  else if (req.indexOf("/gpio/down") != -1)
    sendbutton(down);
  else if (req.indexOf("/gpio/print") != -1)
    sendbutton(prin);
  else if (req.indexOf("/gpio/index") != -1)
    sendbutton(inde);
  else if (req.indexOf("/gpio/rotate") != -1)
    sendbutton(rotate);
  //  else {
  //  Serial.println("invalid request");
  //client.stop();
  //    return;
  //  }

  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n ";
  s += "<h1>Sony Camera Web Remote</h1>";
  s += "<input type=submit value=Shutter style=width:300px;height:150px onclick=location.href='/gpio/shutter'>";
  s += "<input type=submit value=\"2 sec\" style=width:300px;height:150px onclick=location.href='/gpio/2sec'>";
  s += "<input type=submit value=\"START/STOP\" style=width:300px;height:150px onclick=location.href='/gpio/start'>";
  s += "<br>";
  s += "<input type=submit value=DISP style=width:300px;height:150px onclick=location.href='/gpio/disp'>";
  s += "<input type=submit value=Histogram style=width:300px;height:150px onclick=location.href='/gpio/hist'>";
  s += "<input type=submit value=+ style=width:300px;height:150px onclick=location.href='/gpio/zin'>";
  s += "<br>";
  s += "<input type=submit value=\"Image Index\" style=width:300px;height:150px onclick=location.href='/gpio/index'>";
  s += "<input type=submit value=Rotation style=width:300px;height:150px onclick=location.href='/gpio/rotate'>";
  s += "<br>";
  s += "<input type=submit value=Playback style=width:300px;height:150px onclick=location.href='/gpio/playback'>";
  s += "<input type=submit value=\"Slide Show\" style=width:300px;height:150px onclick=location.href='/gpio/slide'>";
  s += "<input type=submit value=- style=width:300px;height:150px onclick=location.href='/gpio/zout'>";
  s += "<br>";
  s += "<input type=submit value=Menu style=width:300px;height:150px onclick=location.href='/gpio/menu'>";
  s += "<input type=submit value=Up style=width:300px;height:150px onclick=location.href='/gpio/up'>";
  s += "<input type=submit value=Trash style=width:300px;height:150px onclick=location.href='/gpio/trash'>";
  s += "<br>";
  s += "<input type=submit value=Left style=width:300px;height:150px onclick=location.href='/gpio/left'>";
  s += "<input type=submit value=Enter style=width:300px;height:150px onclick=location.href='/gpio/enter'>";
  s += "<input type=submit value=Right style=width:300px;height:150px onclick=location.href='/gpio/right'>";
  s += "<br>";
  s += "<input type=submit value=Print style=width:300px;height:150px onclick=location.href='/gpio/print'>";
  s += "<input type=submit value=down style=width:300px;height:150px onclick=location.href='/gpio/down'>";
  // Send the response to the client
  while (s.length() > 2000)
  {
    String dummy = s.substring(0, 2000);
    client.print(dummy);
    s.replace(dummy, " ");
  }

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed


}

