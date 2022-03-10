/*
   GND  - GND
   VCC  - 3.3V
   CE   - pin 9
   CSN  - pin 10
   MOSI - pin 11
   MISO - pin 12
   SCK  - pin 13
   IRQ  - Não conectado
*/

#include <SPI.h>
#include "RF24.h"
#include <printf.h>

// pin CE e CS
RF24 Radio(9, 10);


#define MEUIP 0x00000002
#define MINHAPORTA 10050

byte inputBuffer[32];

const uint64_t enderecos[1] = {"BD"};
byte MeuEnder = 1;
byte Broad = 55;

bool role = 0;            // Identifica se Node esta recebendo ou transmitido

//teste do protocolo de vetor de distancias do projeto final de RDC 2016
#include <stdlib.h>
#include <stdio.h>
/*
struct pacoteIPV4 {
Version   IHL   DSCP  ECN   Total Length
4   32  Identification  Flags   Fragment Offset
8   64  Time To Live  Protocol  Header Checksum
12  96  Source IP Address
16  128   Destination IP Address
}
*/
struct pacote_nossa_redeIPV4{//max 32 bytes
  byte totalLength;//tamanho total do pacote em bytes, incluindo o cabeçalho, minimo 10
  byte timeToLive;//numero de saltos maximo
  int  sourceIPAddress;//endereço de quem enviou o pacote
  int  destinationIPAddress;//endereco do destino do pacote
  byte payload[22];//dados
};

struct pacote_nossa_UDP{//max 22
  byte   totalLength;//tamanho total do pacote em bytes, incluindo o cabeçalho, minimo 5
  short  sourcePort;//porta de quem enviou o pacote
  short  destinationPort;//porta de destino do pacote
  byte   payload[17];//dados para a camada de aplicação
};

struct vetor{//vetor para avaliar a distancia entre os dispositivos
  byte dest     :2;
  byte prox     :2;
  byte num_saltos   :2;
  byte        :0;//completa a variavel
};

struct pacote_nossa_redeIPV4 addIP(byte *payload,byte lenght,int srcIP, int dstIP){
  struct pacote_nossa_redeIPV4 retorno{
  lenght + 6,//tamanho total do pacote em bytes, incluindo o cabeçalho, minimo 6
  6,//numero de saltos maximo
  srcIP,//endereço de quem enviou o pacote
  dstIP,//endereco do destino do pacote
  *payload//dados
  };
  return retorno;
  
  //radio.write*()
}

struct pacote_nossa_redeIPV4 identificaIP(byte *payload){
  struct pacote_nossa_redeIPV4 retorno;
  retorno = *(struct pacote_nossa_redeIPV4*) payload;
  return retorno;
}

struct pacote_nossa_UDP identificaUDP(byte *payload){
  struct pacote_nossa_UDP retorno;
  retorno = *(struct pacote_nossa_UDP*) payload;
  return retorno;
}

void send_ (byte *payload, byte lenght, int srcIP, int dstIP, short srcPort, short dstPort) {
  struct pacote_nossa_UDP withUDP = addUDP(payload, lenght, srcPort, dstPort);
  Serial.print("totalLength =");  Serial.println(withUDP.totalLength);
  Serial.print("sourcePort =");  Serial.println(withUDP.sourcePort);
  Serial.print("destinationPort =");  Serial.println(withUDP.destinationPort);
  Serial.print("payload =");  Serial.println((char*)withUDP.payload);
  struct pacote_nossa_redeIPV4 withIP = addIP((byte*)&withUDP, withUDP.totalLength, srcIP, dstIP);
  Serial.print("totalLength =");  Serial.println(withIP.totalLength);
  Serial.print("timeToLive =");  Serial.println(withIP.timeToLive);
  Serial.print("sourceIPAddress =");  Serial.println(withIP.sourceIPAddress);
  Serial.print("destinationIPAddress =");  Serial.println(withIP.destinationIPAddress);
  Radio.write( &withIP, withIP.totalLength);
}

  
void receive (byte *payload){
  struct pacote_nossa_redeIPV4 withIP = identificaIP(payload);
  Serial.print("totalLength =");  Serial.println(withIP.totalLength);
  Serial.print("timeToLive =");  Serial.println(withIP.timeToLive);
  Serial.print("sourceIPAddress =");  Serial.println(withIP.sourceIPAddress);
  Serial.print("destinationIPAddress =");  Serial.println(withIP.destinationIPAddress);

  if(withIP.destinationIPAddress == MEUIP){
    struct pacote_nossa_UDP withUDP = identificaUDP(withIP.payload);
      Serial.print("totalLength =");  Serial.println(withUDP.totalLength);
      Serial.print("sourcePort =");  Serial.println(withUDP.sourcePort);
      Serial.print("destinationPort =");  Serial.println(withUDP.destinationPort);
      Serial.print("payload =");  Serial.println((char*)withUDP.payload);
      
    if(withUDP.destinationPort == MINHAPORTA){
      Serial.println("Recebido");
    }
  }
}

struct pacote_nossa_UDP addUDP(byte *payload,byte lenght,short srcPort,short dstPort){
  struct pacote_nossa_UDP retorno{//max 22
    lenght + 5,//tamanho total do pacote em bytes, incluindo o cabeçalho, minimo 5
    srcPort,//porta de quem enviou o pacote
    dstPort,//porta de destino do pacote
    payload//dados para a camada de aplicação
  };
  return retorno;
}

void setup()
{
  Serial.begin(115200);
  Serial.println(F("RF24/Transmissor_Receptor/Broadcast"));

  Radio.begin();
  Radio.setChannel(108);
  Radio.setPALevel(RF24_PA_MIN);
  Radio.setDataRate(RF24_1MBPS);
  Radio.setAutoAck(1);                     // Ensure autoACK is enabled
  Radio.setRetries(2, 15);                 // Optionally, increase the delay between retries & # of retries
  Radio.setCRCLength(RF24_CRC_8);          // Use 8-bit CRC for performance

  // Broadcast
  Radio.openWritingPipe(enderecos[0]);
  Radio.openReadingPipe(1, enderecos[0]);
}

void loop()
{
  if (role == 0)
  {
    Radio.startListening();
    if ( Radio.available())       // Verifica se tem algum dado vindo do transmissor
    {
      while (Radio.available())   // Enquanto tiver dado pronto
      {
        Radio.read( inputBuffer, 32 ); // recebe o dado
        receive(inputBuffer);
      }
    }
  }

  if ( role == 1 )
  {
    Radio.stopListening();
    send_("oi2",3,MEUIP,0x00000001,MINHAPORTA,10100);//void send_ (byte *payload,byte lenght,int srcIP,int dstIP,short srcPort,short dstPort){
    Serial.println("Transmitido");
    delay(2000);
  }

  if ( Serial.available() )
  {
    byte c = toupper(Serial.read());
    if ( c == 'T' && role == 0 )
    {
      Serial.println(F("TRANSMISSOR -- PRESSIONE 'R' PARA RECEBER"));
      role = 1;                   // Vira transmissor
    }
    else if ( c == 'R' && role == 1 )
    {
      Serial.println(F("RECEPTOR -- PRESSIONE 'T' PARA TRANSMITIR"));
      role = 0;                  // Vira receptor
    }
  }
}
