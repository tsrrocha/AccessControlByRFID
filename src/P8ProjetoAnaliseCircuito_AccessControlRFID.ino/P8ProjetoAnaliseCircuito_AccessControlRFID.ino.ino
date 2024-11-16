 #include<stdio.h>
#include<SPI.h>
#include<MFRC522.h>


/**  Definicoes do projeto   */
#define LED 13
// Pino de acionamento do rele
#define ATUADOR 7
// Pino de cadastramento de cartao
#define ADD_CARD 6
// Pino de remocao de cartao
#define DEL_CARD 5
// Maximo de Cartoes que podem ser cadastrados
#define MAX_CARD 10

// Estrutura de dados da aplicacao
typedef struct {
  struct {
    unsigned add: 1;    // Estado de ADICIONAR
    unsigned del: 1;
    unsigned read: 1;
  } state;
}TDataApplication;


/**   VARIAVEIS   */
TDataApplication app;

void setup() {
  app.state.add = 0;
  app.state.del = 0;
  app.state.read = 0;

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
}

void loop() {
    // Le a entrada de cadastramento
    app.state.add =  (digitalRead(ADD_CARD)==1)? 1 : 0;
    // Le a entrada de Remocao de cartoes
    app.state.del =  (digitalRead(DEL_CARD)==1)? 1 : 0;
    // Define o estado do READ
    if ((app.state.add == 1) || (app.state.del == 1)) {
      Serial.write ("DEBUG: Eh para fazer cadastro e remocao de cartao\n");
      // É para fazer o cadastramento ou remocao do cartao
      app.state.read = 1;
    } else {
      Serial.write ("DEBUG: Eh para fazer a leitura de cartao\n");
      // É para fazer a leitura do cartao e liberar o acesso
      app.state.read = 0;
    }



  // put your main code here, to run repeatedly:
  if (Serial.available()>0){
    Serial.write("DEBUG...\n");
    digitalWrite(LED, HIGH);
    delay(500);

  }
  
  digitalWrite(LED, LOW);
  delay(100);
  digitalWrite(LED, HIGH);
  delay(100);
}
