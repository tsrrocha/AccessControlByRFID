/**========================================================================================================
 *       Projeto: Leitor de RFID
 *       Disciplina: Análise de Circuitos
 *       Dupla: 
 *          Tiago Sousa Rocha
 *          Jonathan Pereira
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
#define MAX_CARD          10  // Maximo de Cartoes que podem ser cadastrados
#define BASE_TIME_DELAY   200
#define BASE_1S           (1000 / BASE_TIME_DELAY)
#define T_1_SEG           (1 * BASE_1S)
#define T_3_SEG           (3 * BASE_1S)
#define T_5_SEG           (5 * BASE_1S)
#define T_10_SEG          (10 * BASE_1S)

#define TEMPO_LED         T_1_SEG
#define TEMPO_SAIDA       T_5_SEG
#define LED_OK            3   // Define o pino do LED
#define LED_FAIL          2   // Define o pino do LED
#define ATUADOR           7   // Pino de acionamento do rele
#define ADD_CARD          6   // Pino de cadastramento de cartao
#define DEL_CARD          5   // Pino de remocao de cartao
#define RFID_RST_PIN      9   // Define o pino de reset do leitor RFID
#define RFID_SS_PIN       10  // Pino de seleção do leitor RFID

//  RST   -> Pin 9
//  MISO  -> Pin 12
//  MOSI  -> Pin 11
//  SCK   -> Pin 13
//  SDA   -> Pin 10


MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);



/** -------------------------*/
/**  Estrutura de dados      */
/** -------------------------*/
typedef unsigned char BYTE;     // 1 byte
typedef unsigned long DWORD;    // 4 bytes

// Lista Encadeada
struct TNode {
  DWORD cardnumber;
  TNode* next;  
} ;

typedef struct {
  byte qty;
  TNode *first;
} TList;

// Estrutura de dados da aplicacao
typedef struct {
  struct {
    unsigned add: 1;    // Estado de ADICIONAR
    unsigned del: 1;
    unsigned activeOutput: 1;
    unsigned ledOK: 1;
    unsigned ledFail: 1;
  } state;
  DWORD card[MAX_CARD];
  TList cards;
  BYTE qtyCard;
}TDataApplication;


/** -------------------------*/
/**  Variaveis globais       */
/** -------------------------*/
TDataApplication app;
char msg[60];
byte showList = 0;                // flag para sinalizar que deve imprimir a lista
byte cntTimerActiveOutput = 0;    // Contador para temporizar o desacionamento da saida
byte cntTimerLEDOK = 0;           // Contador para temporizar o desacionamento do LED de OK
byte cntTimerLEDFail = 0;         // Contador para temporizar o desacionamento do LED de Fail



/** -------------------------*/
/**  Prototipos de funcao    */
/** -------------------------*/
DWORD verificaNovoCartao();
byte verificaAcessoLiberado(DWORD card);

byte cardExists(TList* lst, DWORD card);
byte addCardIntoList(TList* lst, DWORD card);
byte delCardIntoList(TList * lst, DWORD card);






void setup() {
  Wire.begin();       //INICIALIZA A BIBLIOTECA WIRE
  SPI.begin();        //INICIALIZA O BARRAMENTO SPI
  mfrc522.PCD_Init(); //INICIALIZA MFRC522

  // 
  memset(&app, 0, sizeof(TDataApplication));

  // 
  Serial.begin(9600);
  
  // Configura os pinos que serao saidas (OUTPUT)
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_FAIL, OUTPUT);
  pinMode(ATUADOR, OUTPUT);
  
  // Configura os pinos que serão entradas (INPUT)
  pinMode(ADD_CARD, INPUT_PULLUP);
  pinMode(DEL_CARD, INPUT_PULLUP);

  //
  digitalWrite(LED_OK, LOW);
  digitalWrite(LED_FAIL, LOW);
  digitalWrite(ATUADOR, LOW);

  Serial.write("==============================\tINICIANDO\t================\n");
  sprintf(msg, "Total de cartões cadastrados: %d\n", app.qtyCard);
  Serial.write(msg);
  Serial.write("==============================\t         \t================\n");
}



void loop() {
  DWORD cardRead = 0;

  // Lê o estado das entradas para Operação de Cadastramento ou Remoção de cartão
  app.state.add =  (digitalRead(ADD_CARD)==0)? 1 : 0;
  app.state.del =  (digitalRead(DEL_CARD)==0)? 1 : 0;

  // Lê um cartão RFID
  cardRead = verificaNovoCartao();
  if (cardRead > 0) {
    // Se operação for para Cadastrar
    if (app.state.add == 1) {
      if (app.cards.qty >= MAX_CARD) {
        sprintf(msg, "DEBUG: Memória cheia! Cartão não foi cadastrado. (Qty: %d)\n", app.cards.qty);
        Serial.write(msg);
      } else {
        showList = 1;   // Ativa a impressão da lista
        byte res = addCardIntoList(&app.cards, cardRead);
        if (res == 1) {
          sprintf(msg, "DEBUG: Cartão %lu cadastrado com sucesso! (Qty: %d)\n", cardRead, app.cards.qty);
          Serial.write(msg); 
          app.state.ledOK = 1;
          digitalWrite(LED_OK, HIGH);  
        } else {
          if (res == 2) {
            sprintf(msg, "DEBUG: Cartão %lu já existe! (Qty: %d)\n", cardRead, app.cards.qty);
          } else {
            sprintf(msg, "ERROR: Falha ao cadastrar o cartão: %lu! (Qty: %d)\n", cardRead, app.cards.qty);
            app.state.ledFail = 1;
            digitalWrite(LED_FAIL, HIGH); 
          }
          Serial.write(msg);  
        }
      }
    } else if (app.state.del == 1) {
      // Se operação for de Remover o cartao
      if (app.cards.qty <= 0) {
        sprintf(msg, "DEBUG: Lista vazia! (Qty: %d)\n", app.cards.qty);
        Serial.write(msg);  
      } else {
        showList = 1;   // Ativa a impressão da lista
        if (delCardIntoList(&app.cards, cardRead) == 1) {
          sprintf(msg, "DEBUG: Cartão %lu removido com sucesso! (Qty: %d)\n", cardRead, app.cards.qty);
          Serial.write(msg);  
          app.state.ledOK = 1;
          digitalWrite(LED_OK, HIGH);
        } else {
          sprintf(msg, "ERROR: Falha ao remover o cartão: %lu! (Qty: %d)\n", cardRead, app.cards.qty);
          Serial.write(msg); 
          app.state.ledFail = 1;
          digitalWrite(LED_FAIL, HIGH); 
        }
      }
    } else {
      // Se operação é para liberação de acesso
      if (cardExists(&app.cards, cardRead) == 1) {
        showList = 1;   // Ativa a impressão da lista
        app.state.activeOutput = 1;
        app.state.ledOK = 1;
        digitalWrite(ATUADOR, HIGH);
        digitalWrite(LED_OK, HIGH);
        sprintf(msg, "DEBUG: Acesso Liberado o cartão: %lu! (Qty: %d)\n", cardRead, app.cards.qty);
        Serial.write(msg);
      } else {
        app.state.ledFail = 1;
        digitalWrite(LED_FAIL, HIGH);
        sprintf(msg, "DEBUG: Acesso Negado!\n");
        Serial.write(msg);
      }
    }
  }

  // LED OK
  if (app.state.ledOK == 1) {
    cntTimerLEDOK++;
    if( cntTimerLEDOK >= TEMPO_LED) {
      cntTimerLEDOK = 0;
      app.state.ledOK = 0;
      digitalWrite(LED_OK, LOW);
      Serial.write("DEBUG: DESATIVA LED_OK\n");
    }
  }

  // LED FAIL
  if (app.state.ledFail == 1) {
    cntTimerLEDFail++;
    if( cntTimerLEDFail >= TEMPO_LED) {
      cntTimerLEDFail = 0;
      app.state.ledFail = 0;
      digitalWrite(LED_FAIL, LOW);
      Serial.write("DEBUG: DESATIVA LED_FAIL\n");
    }
  }

  if (app.state.activeOutput == 1) {
    cntTimerActiveOutput ++;
    if(cntTimerActiveOutput >= TEMPO_SAIDA) {
      cntTimerActiveOutput = 0;
      app.state.activeOutput = 0;
      digitalWrite(ATUADOR, LOW);
      Serial.write("DEBUG: DESATIVA SAIDA\n");
    }
  }

  if (showList == 1) {
    showList = 0;
    sprintf(msg, "\n Imprime lista de cartões (Qty: %d):\n", app.cards.qty);
    Serial.write(msg);
    TNode* No = app.cards.first;
    byte idx = 0;
    do {
      if (No != NULL) {
        sprintf(msg, "[%0u] Card: %lu\n", idx, No->cardnumber);
        Serial.write(msg);
      }
      idx++;
      No = No->next;
    }while(No);
  }
  
  
  delay(BASE_TIME_DELAY);
}




DWORD verificaNovoCartao()
{
    DWORD cardnumber = 0;

    sprintf(msg, ""); 
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())  //VERIFICA SE O CARTÃO PRESENTE NO LEITOR É DIFERENTE DO ÚLTIMO CARTÃO LIDO. CASO NÃO SEJA, FAZ
      return 0; 
      
    // Leu um novo cartao RFID
    for (byte i = 0; i < 4; i++) {
      cardnumber = mfrc522.uid.uidByte[i] | (cardnumber << 8);
    }    
    //sprintf(msg, "DEBUG: Cartao lido: %lu \n", cardnumber);      
    //Serial.write(msg);
 
    mfrc522.PICC_HaltA(); //PARADA DA LEITURA DO CARTÃO
    mfrc522.PCD_StopCrypto1(); //PARADA DA CRIPTOGRAFIA NO PCD

    return cardnumber;
};

byte verificaAcessoLiberado(DWORD card)
{
    for (byte i = 0; i < app.qtyCard; i++) {
      if (app.card[i] == card) {
        return 1;
      }
    }
    return 0;
};

byte cardExists(TList* lst, DWORD card) {
  TNode *no;
  if (lst->first == NULL) {
    lst->qty = 0;
    return 0;
  } else {
    no = lst->first;    
    while(no != NULL) {
      if (no->cardnumber == card){
        return 1;
      }
      no = no->next;
    }
    if (no->cardnumber == card){
      return 1;
    }
    return 0;
  }
};

byte addCardIntoList(TList * lst, DWORD card) {
  TNode *no;
  if (cardExists(lst, card) == 0) {
    // Cartão não existe.
    if (lst->qty >= MAX_CARD) {
      return 3;
    } else {
      if (lst->first == NULL) {
        // Lista vazia
        lst->first = (TNode *) malloc (sizeof(TNode));
        lst->first->cardnumber = card;
        lst->first->next = NULL;
        lst->qty = 1;
        return 1;
      } else {
        no = lst->first;
        while(no->next != NULL) {
          no = no->next;
        }
        no->next = (TNode *) malloc (sizeof(TNode));
        no->next->next = NULL;
        no->next->cardnumber = card;
        lst->qty ++;
        return 1;
      }
    }
  } else {
    // Cartão já cadastrado
    return 2;
  }
  return 0;
};

byte delCardIntoList(TList * lst, DWORD card) {
  TNode *noAtual;
  TNode *noAnterior;
  if (cardExists(lst, card) != 0) {
    // Cartão não existe.
    if (lst->qty <= 0) {
      return 0;
    } else {
      noAnterior = NULL;
      noAtual = lst->first;
      do {
        if (noAtual != NULL) {
          if (noAtual->cardnumber == card) {
            // Achou o cartao a remover
            if (noAnterior == NULL) {
              // é o primeiro da fila
              lst->first = noAtual->next;
              lst->qty--;
              free(noAtual);
              return 1;
            } else {
              // está no meio ou fim da fila
              noAnterior->next = noAtual->next;
              lst->qty--;
              free(noAtual);
              return 1;
            }
          } else {
            noAnterior = noAtual;
            noAtual = noAtual->next;
          }
        }
      } while (noAtual);
    }
  }
  return 0;
};
