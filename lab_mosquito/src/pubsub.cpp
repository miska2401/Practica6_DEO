#include <WiFi.h>
#include <TFT_eSPI.h>
#include <PubSubClient.h>
#include "MAX30100_PulseOximeter.h"

const char *ssid = "Samuel’s iPhone";
const char *password = "Dulipasa-123";
const char *mqtt_server = "172.20.10.9";

TFT_eSPI tft = TFT_eSPI();

float spO2 = 0;
float ritmo = 0;

#define BUTTON_LEFT 35

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

bool toggle = true;

PulseOximeter pox;

void isr()
{
    if (toggle)
    {
        Serial.println("on");
    }
    else
    {
        Serial.println("off");
    }

    toggle = !toggle;
}

void setup_wifi()
{

    delay(10);
    Serial.println();
    Serial.print("Conectando a la red: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi Conectado");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Llega mensaje del topic: ");
    Serial.print(topic);
    Serial.print(".. Mensaje ..");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
        messageTemp += (char)payload[i];
    }
    Serial.println();

    if (String(topic) == "esp32/led1")
    {
        if (messageTemp == "1")
        {
            Serial.println("led 1 on");
            tft.fillCircle(210, 40, 15, TFT_YELLOW);
        }
        else if (messageTemp == "0")
        {
            Serial.println("led 1 off");
            tft.fillCircle(210, 40, 15, TFT_DARKGREY);
        }
    }

    if (String(topic) == "esp32/led2")
    {
        if (messageTemp == "1")
        {
            Serial.println("led 2 on");
            tft.fillCircle(210, 90, 15, TFT_GREEN);
        }
        else if (messageTemp == "0")
        {
            Serial.println("led 2 off");
            tft.fillCircle(210, 90, 15, TFT_DARKGREY);
        }
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Intentando conexión MQTT...");

        if (client.connect("ESP8266Client1"))
        {
            Serial.println("conectado");
            // Suscribe
            client.subscribe("esp32/led1");
            client.subscribe("esp32/led2");
        }
        else
        {
            Serial.print("falló, rc=");
            Serial.print(client.state());
            Serial.println(" intentando en 5 segundos");
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(1);

    attachInterrupt(BUTTON_LEFT, isr, RISING);

    Serial.print("Iniciando Pulsioxímetro..");
    if (!pox.begin())
    {
        Serial.println("ERROR");
        for (;;)
            ;
    }
    else
    {
        Serial.println("ÉXITO");
    }
}

void loop()
{
    pox.update();
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 2000)
    {
        lastMsg = now;

        spO2 = pox.getSpO2();
        ritmo = pox.getHeartRate();

        char o2String[8];
        dtostrf(spO2, 2, 0, o2String);
        Serial.print("SpO2: ");
        Serial.println(o2String);
        client.publish("esp32/spo2", o2String);
        tft.fillRect(5, 80, 100, 40, TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("spO2 %:", 5, 65, 2);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(o2String, 5, 80, 6);

        char ritmoString[8];
        dtostrf(ritmo, 3, 0, ritmoString);
        Serial.print("ritmo: ");
        Serial.println(ritmoString);
        client.publish("esp32/ritmo", ritmoString);
        tft.fillRect(5, 15, 100, 40, TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("Ritmo Cardiaco:", 5, 0, 2);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(ritmoString, 5, 15, 6);

        char toggleString[8];
        sprintf(toggleString, "%d", toggle);
        Serial.print("Toggle: ");
        Serial.println(toggleString);
        client.publish("esp32/sw", toggleString);
    }
}