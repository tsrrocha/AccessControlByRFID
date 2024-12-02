/**========================================================================================================
 *       Projeto: Leitor de RFID para Controle de Acesso
 *       Disciplina: Análise de Circuitos
 *       Dupla: 
 *          Tiago Sousa Rocha
 *          Jonathan Pereira
 *       Descricao:
 *          Este circuito permite realizar o controle de acesso através do acionamento de uma saída mediante
 *       A liberação do acesso por meio da leitura de um cartao RFID que esteja cadastrado no sistema.
 *
 *=======================================================================================================*/

/** -------------------------*/
/**  Bibliotecas importadas  */
/** -------------------------*/
//#include<EEPROM.h>
#include<stdio.h>
#include<SPI.h>
#include<MFRC522.h>

/** -------------------------*/
/**  Definicoes do projeto   */
/** -------------------------*/
#define ATUADOR_ENABLED   LOW
#define ATUADOR_DISABLED  HIGH
#define MAX_CARD          10      // Maximo de Cartoes que podem ser cadastrados
#define BASE_TIME_DELAY   200     // Base de tempo para a funcao Loop()
#define BASE_1S           (1000 / BASE_TIME_DELAY)
#define T_1_SEG           (1 * BASE_1S)
#define T_3_SEG           (3 * BASE_1S)
#define T_5_SEG           (5 * BASE_1S)
#define T_10_SEG          (10 * BASE_1S)
#define T_15_SEG          (15 * BASE_1S)
// TEMPOS
#define TEMPO_LED         T_1_SEG
#define TEMPO_SAIDA       T_5_SEG

// PINOS
#define LED_OK            3   // Define o pino do LED de Confirmacao
#define LED_FAIL          2   // Define o pino do LED de Falha
#define LED_ATUADOR       8   // Define o pino do LED do Atuador
#define ATUADOR_1         7   // Pino de acionamento do rele
#define ADD_CARD          5   // Pino de cadastramento de cartao
#define DEL_CARD          4   // Pino de remocao de cartao
#define RFID_RST_PIN      9   // Define o pino de reset do leitor RFID
#define RFID_SS_PIN       10  // Pino de seleção do leitor RFID

// Configura os pinos do modulo RFID usando o protocolo SPI
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

/** -------------------------*/
/**  Estrutura de dados      */
/** -------------------------*/
typedef unsigned char BYTE;     // 1 byte
typedef unsigned long DWORD;    // 4 bytes

// Estrutura TNode para armazenar o cartao cadastrado e
// manter o apontamento para o proximo cartao.
struct TNode {
  DWORD cardnumber;
  TNode* next;  
} ;

// Lista Simplesmente Encadeada para listar os cartoes cadastrados.
// No maximo MAX_CARD.
typedef struct {
  byte qty;
  TNode *first;
} TList;

// Estrutura de dados da aplicacao
typedef struct {
  struct {
    unsigned add: 1;          // flag para sinalizar a acao de ADICIONAR cartao
    unsigned del: 1;          // flag para sinalizar a acao de REMOVER cartao
    unsigned activeOutput: 1; // flag para sinalziar a ativacao do atuador
    unsigned ledOK: 1;        // flag para sinalizar a ativacao do led de confirmacao
    unsigned ledFail: 1;      // flag para sinalizar a ativacao do led de falha
    unsigned showList: 1;     // flag para exibir a lista de cartoes
  } state;
  TList cards;                // Lista de cartoes
  BYTE qtyCard;               // Quantidade de cartoes na lista (cadastrados)
}TDataApplication;

/** -------------------------*/
/**  Variaveis globais       */
/** -------------------------*/
TDataApplication app;             // Estrutura de configuracao e estado da aplicacao
char msg[60];                     // Array de caracteres para exibicao de mensagens na Serial
byte cntTimerActiveOutput = 0;    // Contador para temporizar o desacionamento da saida
byte cntTimerLEDOK = 0;           // Contador para temporizar o desacionamento do LED de OK
byte cntTimerLEDFail = 0;         // Contador para temporizar o desacionamento do LED de Fail

/** -------------------------*/
/**  Prototipos de funcao    */
/** -------------------------*/
// Funcao de leitura de um novo cartao RFID
DWORD verificaNovoCartao();
// Funcao que verifica se o cartao (card) já está cadastrado
byte cardExists(TList* lst, DWORD card);
// Funcao que adiciona o cartao (card) na lista (lst)
byte addCardIntoList(TList* lst, DWORD card);
// Funcao que remove o cartao (card) da lista (lst)
byte delCardIntoList(TList * lst, DWORD card);

// Funcao de configuracao da Aplicacao
void setup() {
  SPI.begin();        // Inicializa a biblioteca SPI
  mfrc522.PCD_Init(); // Inicializa o modulo MFRC522

  // Set todos os parâmetros da variavel app com valor 0
  memset(&app, 0, sizeof(TDataApplication));

  // Inicializa a Serial usando um Baudrate de 9600 bps
  Serial.begin(9600);
  
  // Configura os pinos que serao saidas (OUTPUT)
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_FAIL, OUTPUT);
  pinMode(LED_ATUADOR, OUTPUT);
  pinMode(ATUADOR_1, OUTPUT);
  
  // Configura os pinos que serão entradas (INPUT)
  pinMode(ADD_CARD, INPUT_PULLUP);
  pinMode(DEL_CARD, INPUT_PULLUP);

  // Define o estado inicial das saidas
  digitalWrite(LED_OK, LOW);
  digitalWrite(LED_FAIL, LOW);
  digitalWrite(LED_ATUADOR, LOW);
  digitalWrite(ATUADOR_1, ATUADOR_DISABLED);

  Serial.write("==============================\tINICIANDO\t================\n");
  sprintf(msg, "Total de cartões cadastrados: %d\n", app.qtyCard);
  Serial.write(msg);
  Serial.write("==============================\t         \t================\n");
}


// Funcao principal da aplicacao Loop()
void loop() {
  DWORD cardRead = 0;

  // Lê o estado das entradas para Operação de Cadastramento ou Remoção de cartão
  app.state.add =  (digitalRead(ADD_CARD)==0)? 1 : 0;
  app.state.del =  (digitalRead(DEL_CARD)==0)? 1 : 0;

  // Verifica se há um novo cartão RFID a ser lido
  cardRead = verificaNovoCartao();
  if (cardRead > 0) { // Sim
    // Se operação for para Cadastrar
    if (app.state.add == 1) {
      if (app.cards.qty >= MAX_CARD) {
        sprintf(msg, "DEBUG: Memória cheia! Cartão não foi cadastrado. (Qty: %d)\n", app.cards.qty);
        Serial.write(msg);
      } else {
        app.state.showList = 1;   // Ativa a impressão da lista
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
    } 
    // Se a operacao for para Remover o cartao
    else if (app.state.del == 1) {
      // Se operação for de Remover o cartao
      if (app.cards.qty <= 0) {
        sprintf(msg, "DEBUG: Lista vazia! (Qty: %d)\n", app.cards.qty);
        Serial.write(msg);  
      } else {
        app.state.showList = 1;   // Ativa a impressão da lista
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
    } 
    // É uma tentativa de acesso para o cartao RFID lido
    else {
      // Se operação é para liberação de acesso
      if (cardExists(&app.cards, cardRead) == 1) {
        cntTimerActiveOutput = 0;   // Reinicia a contagem do tempo de acesso
        app.state.showList = 1;   // Ativa a impressão da lista
        app.state.activeOutput = 1;
        app.state.ledOK = 1;
        digitalWrite(ATUADOR_1, ATUADOR_ENABLED);
        digitalWrite(LED_OK, HIGH);
        digitalWrite(LED_ATUADOR, HIGH);
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

  // ATUADOR
  if (app.state.activeOutput == 1) {
    cntTimerActiveOutput ++;
    if(cntTimerActiveOutput >= TEMPO_SAIDA) {
      cntTimerActiveOutput = 0;
      app.state.activeOutput = 0;
      digitalWrite(ATUADOR_1, ATUADOR_DISABLED);
      digitalWrite(LED_ATUADOR, LOW);
      Serial.write("DEBUG: DESATIVA SAIDA\n");
    }
  }

  if (app.state.showList == 1) {
    app.state.showList = 0;
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
} // Loop ()


/**
 *  DWORD verificaNovoCartao()
 *  @brief: Esta função faz a leitura/verificação de um novo cartão RFID retornando 
 *  o ID do cartão que é valor dele do tipo DWORD (4 bytes)
 *  
 */
DWORD verificaNovoCartao()
{
    DWORD cardnumber = 0;
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())  //VERIFICA SE O CARTÃO PRESENTE NO LEITOR É DIFERENTE DO ÚLTIMO CARTÃO LIDO. CASO NÃO SEJA, FAZ
      return 0; 
      
    // Leu um novo cartao RFID (4 bytes)
    for (byte i = 0; i < 4; i++) {
      cardnumber = mfrc522.uid.uidByte[i] | (cardnumber << 8);
    }
    mfrc522.PICC_HaltA();       // Para a leitura do cartao
    mfrc522.PCD_StopCrypto1();  // Para a criptografia no PCD
    return cardnumber;
};

/**
 *  byte cardExists(TList* lst, DWORD card)
 *  @brief Esta função percorre a lista simplesmente encadeada (lst) para verificar se já
 *  existe um cartao cadastrado com esse ID (card), retornando 1 se existe, 0 do contrário.
 *  
 */
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

/**
 *  byte addCardIntoList(TList * lst, DWORD card)
 *  @brief Esta função adiciona o cartao (card) no final da lista simplesmente encadeada (lst)
 *  retornando:
 *    0: Se falhou ao cadastrar o cartão;
 *    1: Se foi adicionado com sucesso;
 *    2: Se o cartão já estiver cadastrado;
 *    3: Se a lista estiver cheia.
 * 
 */
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

/**
 *  byte delCardIntoList(TList * lst, DWORD card)
 *  @brief Esta função remove o cartao (card) da lista simplesmente encadeada (lst)
 *  retornando:
 *    0: Se a lista estiver vazia ou se ocorreu algum erro;
 *    1: Se foi removido com sucesso;
 * 
 */
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
