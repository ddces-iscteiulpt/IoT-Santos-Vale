#include <SPI.h>
#include <EmonLib.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
WiFiSSLClient wiFiSSLClient;
MqttClient mqttClient(wiFiSSLClient);
#include "login_secrets.h"
char ssid[] = SECRET_SSID;         // your network SSID (name)
char pass[] = SECRET_PASS;         // your network password (use for WPA, or use as key for WEP)
char usermqtt[] = SECRET_USERMQTT; // your mqtt user
char passmqtt[] = SECRET_PASSMQTT; // your mqtt pass
char broker[] = MQTT_BROKER;
int  port     = MQTT_PORT;
char topic[]  = MQTT_TOPIC;
const long interval = TIMER2SEND * 60 * 1000;

char unidades_medida[] = MEASUREMENT_TYPE;
EnergyMonitor emon1;
int    pin_emon1 = EMON_PIN_1;
double calibration_emon1 = EMON_CALI_1;
EnergyMonitor emon2;
int    pin_emon2 = EMON_PIN_2;
double calibration_emon2 = EMON_CALI_2;
EnergyMonitor emon3;
int    pin_emon3 = EMON_PIN_3;
double calibration_emon3 = EMON_CALI_3;

int Vrms = VOLT;
double Energy = 0;

String Energy_KWh;
String Power_KW;
String ACPhase_1;
String ACPhase_2;
String ACPhase_3;

uint32_t ENERGY;
uint32_t POWER;
uint32_t IRMS1;
uint32_t IRMS2;
uint32_t IRMS3;

unsigned long previousMillis = 0;
int count = 0;
int amostras; //numero de amostras recolhidas antes de enviar
int ind = 6;   //indice de trnsmição 6x10 = 60min (medições de corrente de 1h)

String msg;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  //while (!Serial) {    ; // wait for serial port to connect. Needed for native USB port only  } /////////////////////////////////////
  
  connect_wifi();   // Função de connectar à rede Wi-Fi
  
  connect_mqtt();   // Função de connectar ao broker MQTT cloud

  /*Calibração dos valores //////////////////////////////////////////////////////////////////////////////////////////*/
  /*analogReadResolution é a resolução das entradas ADC, varia com o tipo de transformador quando usado no MKR 1300*
    .current(entrada_ADC, valor calibração) */ //analogReadResolution(9);  
  emon1.current(pin_emon1, calibration_emon1);             // Current: input pin, calibration.
  emon2.current(pin_emon2, calibration_emon2);             // Current: input pin, calibration.
  emon3.current(pin_emon3, calibration_emon3);             // Current: input pin, calibration. 
  /*Calibração dos valores //////////////////////////////////////////////////////////////////////////////////////////*/

  /*  Calibração inicial e envio do 1º valor  */
  for (int i = 1; i <= 10; i++) {
    dataEnergyMonitor();
  }
  /*  Calibração inicial e envio do 1º valor  */

  Energy_KWh  =   String("0")   +   String("E");       //1º valor a receber de energia é sempre 0, memopoint na database

  sendDataMQTT();
}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();

  // avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();

  dataEnergyMonitor();
  
  if (currentMillis - previousMillis >= interval) {    
    // save the last time a message was sent
    previousMillis = currentMillis;
    Serial.println(previousMillis / 1000);
    sendDataMQTT();
  }
}

void connect_wifi()
{
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println("You're connected to the Wi-Fi network");
  Serial.println();
}

void connect_mqtt()
{
  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  //mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword(usermqtt, passmqtt);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}


/* ___________________________________________________________________________________________________________________________________ */
void dataEnergyMonitor() {

  amostras++;
  
  /* Calcular a potencia
    emon.calcIrms (Numero de samples = N), quanto maior o numero de samples, mais tempo demora a calcular,
    mais preciso fica, mas lê o ADC esse N numero de vezes o que gasta bateria
    reduzir consumo na medição é aumentar as 2 resistencias fisicas até 470k ohm*/

  double Irms1 = emon1.calcIrms(1480);
  if (Irms1 < 0.10) Irms1 = 0; //0.05 é o valor minimo de corrente em Amperes, neste caso este codigo foi usado para medir o AVAC portanto 0.10 é o dobro do valor minimo de medida(+-0,05)
  double Irms2 = emon2.calcIrms(1480);
  if (Irms2 < 0.10) Irms2 = 0;
  double Irms3 = emon3.calcIrms(1480);
  if (Irms3 < 0.10) Irms3 = 0;

  double Watt1 = (Irms1 * Vrms) / 1000;
  double Watt2 = (Irms2 * Vrms) / 1000;
  double Watt3 = (Irms3 * Vrms) / 1000;
  double Power = (Watt1 + Watt2 + Watt3);

  Energy = Energy + Power;

  if (ind == 6 )
  {
    Energy = Energy / amostras;  
  }
  Serial.print("Teste indices: ind = ");
  Serial.print(ind);
  Serial.print(" amostra = ");
  Serial.println(amostras);
  
  Serial.print("  AC Fase1: ");
  Serial.print(Irms1);
  Serial.print("; AC Fase2: ");
  Serial.print(Irms2);
  Serial.print("; AC Fase3: ");
  Serial.println(Irms3);  
  Serial.print("    Ultima medição de Potencia(kW): ");
  Serial.println(Power);
  Serial.print("      Energia Consumida na ultima hora(kWh): ");
  Serial.println(Energy);

  ENERGY    =   Energy    *   100;
  POWER     =   Power     *   100;
  IRMS1     =   Irms1     *   100;
  IRMS2     =   Irms2     *   100;
  IRMS3     =   Irms3     *   100;

  Energy_KWh  =   String(ENERGY)   +   unidades_medida[0];
  Power_KW    =   String(POWER)    +   unidades_medida[1];
  ACPhase_1   =   String(IRMS1)    +   unidades_medida[2];
  ACPhase_2   =   String(IRMS2)    +   unidades_medida[3];
  ACPhase_3   =   String(IRMS3)    +   unidades_medida[4];
 
  /*total de kW, para passar para kWh é só dividir este valor pelo numero de intervalo de medições por hora,
    ou seja, se medir de minuto a minuto é dividir por 60.
    O ideal é mesmo meter a dividir por 60 aqui para enviar logo kWh que é a unidade padrão,
    de momento a divisão por 60 é feita no servidor*/

}/* ___________________________________________________________________________________________________________________________________ */

/* ___________________________________________________________________________________________________________________________________ */
void sendDataMQTT()
{
  /* Envia um string com o valor da sequencia e o total de kW - "SEQ;KW"
    Ideal é enviar 4 ou 8 bytes em vez de string para gastar menos, ver código base do MKR 1300, que tem lá como enviar os pacotes em byte*/
    
  if (ind == 6) {
    msg = Energy_KWh + Power_KW + ACPhase_1 + ACPhase_2 + ACPhase_3;
    amostras = 0;
    Energy = 0;     ////////////????????????????    
    ind = 1;     
  }
  else {
    msg =  Power_KW + ACPhase_1 + ACPhase_2 + ACPhase_3;  
  }
 
    Serial.print("Sending message to topic: ");
    Serial.println(topic);
    Serial.print("msg = ");
    Serial.println(msg);

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(msg);
    mqttClient.endMessage();

    Serial.println();

    count++;
}/* ___________________________________________________________________________________________________________________________________ */
