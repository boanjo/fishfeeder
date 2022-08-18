#include <ESP8266WiFi.h>

# error You need to update the 2 lines with your Wifi settings before programming
const char* ssid = "YOUR_SSID";
const char* password = "verysecretpassword";

long start_time = 0;
bool stateFeeding = false;
long pinStartMicros = 0;
long pulseCount = 0;
long previousRssi = 0;
long minRssi = 0;
long maxRssi = -100;
long previousMillis = 0;
int servoPin = D3;
int feedAmountPulses = 50;
int count  = 0;
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);


  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}

void loop() {



  if (stateFeeding) {
    if (digitalRead(servoPin) == LOW) {
      if (micros() - pinStartMicros > 20000) {
	pulseCount++;
	if (pulseCount > feedAmountPulses) {
	  stateFeeding = false;
	  Serial.print(millis() - start_time);
	  Serial.println("ms, Feeding ready");
	} else {
	  digitalWrite(servoPin, HIGH);
	  pinStartMicros = micros();
	}
      }
    }
    else {
      if (micros() - pinStartMicros > 2000) {
	digitalWrite(servoPin, LOW);
	pinStartMicros = micros();
      }
    }
  }
  else {
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    previousRssi = rssi;
    if (rssi > maxRssi) maxRssi = rssi;
    if (rssi < minRssi) minRssi = rssi;
    if ((unsigned long)(millis() - previousMillis) >= 5000)
      {
	Serial.print("RSSI:");
	Serial.print(rssi);
	Serial.print(" MIN:");
	Serial.print(minRssi);
	Serial.print(" MAX:");
	Serial.println(maxRssi);
	previousMillis = millis();
      }
  }
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  long client_time = millis();
  while (!client.available()) {
    delay(1);

    if ((millis() - client_time) > 10) {
      Serial.println("Abort no data after 10ms");
      return;
    }
  }

  Serial.println("waiting for available");

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request

  if (request.indexOf("/FEED=") != -1) {

    String amount = "";
    Serial.print(request.indexOf("="));
    Serial.print(", ");
    Serial.print(request.length());
    Serial.print(", ");
    Serial.println(request);
    for (int i = request.indexOf("=") + 1; i < request.length(); i++) {
      if (isDigit(request.charAt(i))) {
	amount += (char)request.charAt(i);
      } else {
	break;
      }
    }

    int feedAmount = 0;
    if (amount.length() > 0) {
      feedAmount = amount.toInt();
    } else {
      feedAmount = 1;
    }

    Serial.print("FeedAmount = ");
    Serial.print(feedAmount);
    Serial.println("ml");

    if (stateFeeding == true) {
      // 8 pulses per ml
      feedAmountPulses += feedAmount * 8;
    } else {
      // 8 pulses per ml
      feedAmountPulses = feedAmount * 8;
      stateFeeding = true;
      pulseCount = 0;
      digitalWrite(servoPin, HIGH);
      pinStartMicros = micros();
      start_time = millis();
    }

  }


  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one

  // Actual page
  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("<title>FishFeeder 2.0</title>");
  client.println("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"> ");
  client.println("<meta charset=\"utf8\" />");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=0.6\">");
  client.println("<link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css\"/>");
  client.println("<script src=\"http://code.jquery.com/jquery-1.11.1.min.js\"></script>");
  client.println("<script src=\"http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js\"></script>");
  client.println("</head>");
  client.println("<body>");

  // header
  client.println("<div data-role=\"page\">");
  client.println("<div data-role=\"header\"> ");
  client.println("<h1>FishFeeder 2.0</h1> ");
  client.println("</div>");

  // main body
  client.println("<div role=\"main\" class=\"ui-content\">");
  // Important use rel=external to force the page to be refreshed (or the button can only be clicked once)
  client.println("<a href=\"/FEED=10\" rel=\"external\" class=\"ui-btn ui-btn-inline ui-btn-b\">Feed 10ml</a>");
  client.println("<a href=\"/FEED=20\" rel=\"external\" class=\"ui-btn ui-btn-inline ui-btn-b\">Feed 20ml</a>");
  client.println("<a href=\"/FEED=50\" rel=\"external\" class=\"ui-btn ui-btn-inline ui-btn-b\">Feed 50ml</a>");
  client.println("<br>");
  client.print("<p>Hint: You can feed by calling ");
  client.print(WiFi.localIP());
  client.println("/FEED=30 (as an example for 30ml)</p>");
  client.print("<p>");
  client.print("Wifi Statistics: RSSI:");
  client.print(previousRssi);
  client.print(" MIN:");
  client.print(minRssi);
  client.print(" MAX:");
  client.print(maxRssi);
  client.println("</p>");
  client.println("</div>");

  // footer
  client.println("<div data-role=\"footer\"> ");
  client.println("<h4>&copy; 2020 - Anders Boan Johansson</h4> ");
  client.println("</div>");
  client.println("</div>");
  client.println("</body>");
  client.println("</html>");
  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");



}
