#include "mbed.h"
#include "WizFi310Interface.h"
#include "DHT.h"
#include "MQTTClient.h"
#include "MQTTmbed.h"
#include "MQTTSocket.h"

#define SECURE WizFi310::SEC_WPA2_MIXED
#define SSID "CDI"
#define PASS "Cdi*1717"
/*
    SET DHCP
*/
#define USE_DHCP    1
//--------- Have to modify the mac address-------------
unsigned char MAC_Addr[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};
#if defined(TARGET_WIZwiki_W7500)
    WizFi310Interface wizfi310(D1, D0, D7, D6, D8, NC, 115200);
    Serial pc(USBTX, USBRX);
#endif

int arrivedcount = 0;
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

class MQTTWIZ: public MQTTSocket
{
public:    
    MQTTWIZ()
    {
        wait(1);
        this->createSocket();
    }
};

int main() {


 
   pc.baud(115200);
 
    printf("WizFi310  STATION. \r\n");
    wizfi310.init();
    printf("After Initialisation. \r\n");

    printf("After Set Address. \r\n");
    if ( wizfi310.connect(SECURE, SSID, PASS, WizFi310::WM_STATION))      return -1;
    printf("After Connect. \r\n");
    printf("IP Address is %s\r\n", wizfi310.getIPAddress()); 
    
    MQTTWIZ ipstack = MQTTWIZ();
    MQTT::Client<MQTTWIZ, Countdown> client = MQTT::Client<MQTTWIZ, Countdown>(ipstack);
    
    DHT sensor(D14, DHT11);
    char* hostname = "iot.eclipse.org";
    int port = 1883;
    
    int rc = ipstack.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\n", rc);
    printf("rc from TCP connect is %d\n", rc);
    
    char MQTTClientID[30];
    
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.MQTTVersion = 3;
    sprintf(MQTTClientID,"WIZwiki-W7500-client-%d",rand()%1000);
    data.clientID.cstring = MQTTClientID;
    data.username.cstring = "testuser";
    data.password.cstring = "testpassword";  

    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\n", rc);
        
    MQTT::Message message;
    char buf[100];
    int error = 0;
    float hum = 0.0f, temp = 0.0f;
    char i = 0;
    while (true)
    {
        if(i > 100) i = 0;
        error = sensor.readData();
        if (0 == error) {
            hum = sensor.ReadHumidity();
            temp = sensor.ReadTemperature(CELCIUS);
        }
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        
        sprintf(buf, "%3.1f", hum);
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf);
        rc = client.publish("Wiz/humidity", message);
        printf("Wiz/humidity : %s\r\n",message.payload);
        
        sprintf(buf, "%3.1f", temp);
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf);
        rc = client.publish("Wiz/temperature", message);
        printf("Wiz/temperature : %s\r\n",message.payload);
        client.yield(1000);
    }
}
