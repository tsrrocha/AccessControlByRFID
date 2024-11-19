/**========================================================================================================
 *       Projeto: Leitor de RFID
 *       Disciplina: Análise de Circuitos
 *       Dupla: 
 *          Tiago Sousa Rocha
 *          Jonatan
 *       Descricao:
 *          Este circuito permite realizar o controle de acesso através do acionamento de uma saída mediante
 *       A liberação do acesso por meio da leitura de um cartao RFID que esteja cadastrado no sistema.
 *
 *=======================================================================================================*/
#include<EEPROM.h>
#include<Wire.h>
#include<stdio.h>
#include<SPI.h>
#include<MFRC522.h>

/** -------------------------*/
/**  Definicoes do projeto   */
/** -------------------------*/
// Maximo de Cartoes que podem ser cadastrados
#define MAX_CARD 10

#define LED 8             // Define o pino do LED
#define ATUADOR 7         // Pino de acionamento do rele
#define ADD_CARD 6        // Pino de cadastramento de cartao
#define DEL_CARD 5        // Pino de remocao de cartao
#define RFID_RST_PIN 9    // Define o pino de reset do leitor RFID
#define RFID_SS_PIN 10    // Pino de seleção do leitor RFID
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

//  RST   -> Pin 9
//  MISO  -> Pin 12
//  MOSI  -> Pin 11
//  SCK   -> Pin 13
//  SDA   -> Pin 10

/** -------------------------*/
/**  Estrutura de dados      */
/** -------------------------*/
typedef unsigned char BYTE;
typedef unsigned long DWORD;

// Estrutura de dados da aplicacao
typedef struct {
  struct {
    unsigned add: 1;    // Estado de ADICIONAR
    unsigned del: 1;
    unsigned read: 1;
  } state;
  DWORD card[MAX_CARD];
}TDataApplication;

/** -------------------------*/
/**  Variaveis globais       */
/** -------------------------*/
TDataApplication app;
char msg[40];


/** -------------------------*/
/**  Prototipos de funcao    */
/** -------------------------*/
DWORD verificaNovoCartao();

void setup() {
  Wire.begin();       //INICIALIZA A BIBLIOTECA WIRE
  SPI.begin();        //INICIALIZA O BARRAMENTO SPI
  mfrc522.PCD_Init(); //INICIALIZA MFRC522
  
  app.state.add = 0;
  app.state.del = 0;
  app.state.read = 0;
  app.card[0] = 0;

  // put your setup code here, to run once:
  Serial.begin(9600);
  // Configura os pinos que serao saidas (OUTPUT)
  pinMode(LED, OUTPUT);
  pinMode(ATUADOR, OUTPUT);
  // Configura os pinos que serão entradas (INPUT)
  pinMode(ADD_CARD, INPUT);
  pinMode(DEL_CARD, INPUT);

  //
  digitalWrite(LED, HIGH);

  Serial.write("==============================\tINICIANDO\t================\n");
  /*Serial.write("SizeOf DWORD: ");
  sprintf(msg, "SizeOf: %d\n", sizeof(DWORD));
  Serial.write(msg);
  //*/
}

void loop() {
    DWORD cardRead = 0;
  
    // Le a entrada de cadastramento
    app.state.add =  (digitalRead(ADD_CARD)==1)? 1 : 0;
    // Le a entrada de Remocao de cartoes
    app.state.del =  (digitalRead(DEL_CARD)==1)? 1 : 0;
    // Define o estado do READ
    if ((app.state.add == 1) || (app.state.del == 1)) {
      //Serial.write ("DEBUG: Eh para fazer cadastro e remocao de cartao\n");
      // É para fazer o cadastramento ou remocao do cartao
      app.state.read = 1;
    } else {
      //Serial.write ("DEBUG: Eh para fazer a leitura de cartao\n");
      // É para fazer a leitura do cartao e liberar o acesso
      app.state.read = 0;
    }

    
    cardRead = verificaNovoCartao();
    if (cardRead > 0) {
      // Verifica se é para Adicionar o cartao lido?
      // Verifica se é para Remover o cartao lido?
      // Verifica se é para checar o acesso para o cartao lido?
    }
    
  
    delay(100);
}




DWORD verificaNovoCartao()
{
    DWORD cardnumber = 0;

    sprintf(msg, ""); 
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())  //VERIFICA SE O CARTÃO PRESENTE NO LEITOR É DIFERENTE DO ÚLTIMO CARTÃO LIDO. CASO NÃO SEJA, FAZ
      return 0; 
      
    // Leu um novo cartao RFID
    String strID = ""; 
    //Serial.write("DEBUG: [");
    for (byte i = 0; i < 4; i++) {
      cardnumber = mfrc522.uid.uidByte[i] | (cardnumber << 8);
      strID += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX) + (i!=3 ? ":" : "");
      //sprintf(msg, "(%d) ", mfrc522.uid.uidByte[i]);      
      //Serial.write(msg);
    }    
    //Serial.write("]\n");

    sprintf(msg, "DEBUG: Cartao lido: %lu \n", cardnumber);      
    Serial.write(msg);
    
    //strID.toUpperCase();
    //char msg[30];
    //sprintf(msg, "DEBUG: Novo cartao lido: %s\n", strID);
    //Serial.write(msg);


    mfrc522.PICC_HaltA(); //PARADA DA LEITURA DO CARTÃO
    mfrc522.PCD_StopCrypto1(); //PARADA DA CRIPTOGRAFIA NO PCD

    return cardnumber;
}
