/* ###################################################################
**     Filename    : main.c
**     Project     : EA076 - Projeto 2 - Item 8 a 10
**     Processor   : MKL25Z128VLK4
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2020-12-28, 20:43, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief Main do Projeto
** @mainpage EA076 - Projeto 2 : Comunicacao dos Objetos Inteligentes
** @authors Guilherme Bithencourt Martinelli (168923) e Joao Victor Gitti Aredes (170715) - Grupo 06   
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "Temp.h"
#include "AdcLdd1.h"
#include "ESP.h"
#include "ASerialLdd1.h"
#include "PC.h"
#include "ASerialLdd2.h"
#include "A1.h"
#include "BitIoLdd1.h"
#include "A2.h"
#include "BitIoLdd2.h"
#include "Power.h"
#include "PwmLdd1.h"
#include "TU1.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "TU2.h"
#include "UTIL1.h"
#include "MCUC1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

/* User includes (#include below this line is not maintained by Processor Expert) */

/*!
 * @brief Flag para sinalizar que se deve fazer uma nova amostragem da temperatura
 */
volatile char flag_temp_update = 0;

/*!
** @brief buffer da mensagem recebida
*/
volatile uint8_t rcv[100] = {0}; 
	
/*!
** @brief posicao no buffer rcv
*/
volatile uint8_t posit = 0; 
	
/*!
** @brief flag para sinalizar que a mensagem foi recebida
*/
volatile char message = 0; 

/*!
** @brief Enviar comando para o ESP e recebe o caracter da string como argumento
*/
void SendCmd(uint8_t * s){ 
	while(*s){ //Permanece dentro do while até o caracter da string de comando não for 0 (fim da string)
		while(ESP_SendChar(*s)); //Envia o caracter para o ESP
			s++; //incrementa a posição em que o ponteiro está apontando
	}
}
	
/*!
** @brief Enviar string para o PC e recebe o caracter da string como argumento - Mesma ideia da funcao SendCmd 
*/
void SendPC(uint8_t * s){
	while(*s){
		while(PC_SendChar(*s));	
			s++;
	}
}

	/*!
	** @brief buffer de comando a ser escrito
	*/
	uint8_t cmd[100] = {0}; 
	
	/*!
	** @brief string do topico
	*/
	uint8_t top[50] = {0};
		
	/*!
	** @brief string do parametro do topico
	*/
	uint8_t par[10] = {0};
		
	/*!
	** @brief scanner 1 de string
	*/
	uint8_t scan = 0; 
		
	/*!
	** @brief scanner 2 de string
	*/
	uint8_t scan_2 = 0; 
	
	/*!
	** @brief valor da conversao A/D
	*/
	uint16_t val; 
	
	/*!
	** @brief valor da tensao em mV a partir do valor convertido A/D
	*/
	uint32_t tensao; 
	
	/*!
	** @brief valor da temperatura em decimos de graus Celsius a partir do valor da tensao
	*/
	uint16_t temp;
	
	/*!
	** @brief variavel auxiliar da temperatura
	*/
	uint16_t tempaux;
	
	/*!
	** @brief valor da temperatura limiar
	*/
	uint16_t templimiar;
	
	/*!
	** @brief Variavel temporaria para armazenar o valor da temperatura limiar
	*/
	uint16_t templimiartemp;
	
	/*!
	** @brief Caractere temporario durante a conversao de valor para string
	*/
	uint8_t c;
	
	/*!
	** @brief Modo de operacao do ventilador: 0 - Desligado; 1 - Ligado; 2 - Auto
	*/
	uint8_t mode = 0;
	
	/*!
	** @brief Potencia de operacao do motor do ventilador - valor entre 0 e 100
	*/
	uint8_t power = 0;
	
	/*!
	** @brief Variavel temporaria para armazenar o valor da potencia do motor
	*/
	uint8_t powertemp = 0;
	
	/*!
	** @brief Variavel para contagem nos loops
	*/
	uint8_t i;
	
	/*!
	** @brief Flag para indicar que houve mudanca no modo de operacao do motor
	*/
	uint8_t flagmode = 0;
	
	/*!
	** @brief Indica o sentido de rotacao do motor: 0 - Ventilador ; 1 - Exaustor
	*/
	uint8_t dir = 0;
	
	/*!
	** @brief Flag para indicar que houve mudanca no sentido de rotacao do motor
	*/
	uint8_t flagdir = 0;

	/*!
	** @brief Flag para indicar que houve mudanca na valocidade de rotacao do motor
	*/
	uint8_t flagpower = 0;
	
	/*!
	** @brief Variavel para indicar se houve inscricao nos 4 topicos do MQTT
	*/
	uint8_t count_sub = 4; 

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
/*!
** @brief Faz a conexao ao Wifi e MQTT; Faz a medicao de temperatura e sua publicacao; Trata os comandos para o motor
*/
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  /* For example: for(;;) { } */
  strcpy(cmd, "CONNWIFI \"nome_da_rede\",\"senha_da_rede\"\r\n"); // Comando automático para conexão com a rede wifi
  SendCmd(cmd); //Envia o comando da conexão wifi para o ESP
  while(message == 0); //Certificar-se que o ESP recebeu a mensagem, pois o código sairá desse while quando a flag message = 1 (mensagem recebida)
  message = 0; //Zera a flag
  
  if (UTIL1_strcmp(rcv, "CONNECT WIFI\r\n")){ //Comparação da string de resposta do ESP com a CONNECT WIFI (0- VERDADEIRO, 1- FALSO)
 	  SendPC("WIFI NAO CONECTA\r\n"); //Escreve no PC que não conectou no WIFI
   } else { //Caso as duas strings sejam iguais
   	  strcpy(cmd, "CONNMQTT \"137.135.83.217\",1883,\"client_ID\"\r\n"); // Comando automático para conexão com o servidor broker MQTT
   	  SendCmd(cmd); // Envia o comando da conexão broker MQTT para o ESP
   	  while(message == 0);
   	  message = 0;
   	  if(UTIL1_strcmp(rcv, "CONNECT MQTT\r\n")) { //Comparação da string de resposta do ESP com a CONNECT MQTT (0- VERDADEIRO, 1- FALSO)
   		  SendPC("MQTT NAO CONECTA\r\n"); //Escreve no PC que não conectou no MQTT
   	  } else {
   		  
   		  	strcpy(cmd, "SUBSCRIBE \"EA076/168923/dir\"\r\n"); //Comando para assinar o topico de Direcao (sentido de rotacao)
   		  	SendCmd(cmd); //Envia o comando da assinatura de tópico para o ESP
   		  	while(message == 0);
   		  	message = 0;
   		    if(UTIL1_strcmp(rcv, "OK SUBSCRIBE\r\n")) { //Comparação da string de resposta do ESP com o OK SUBSCRIBE (0- VERDADEIRO, 1- FALSO)
   		  		SendPC("ERRO NO SUBSCRIBE DO DIR\r\n"); //Escreve no PC que ocorreu erro no subscribe
   		  		count_sub--; //1 dos 4 tppicos para subscribe não aconteceu
   		  	} 
   		  	  		 
   		  	strcpy(cmd, "SUBSCRIBE \"EA076/168923/power\"\r\n"); //Comando para assinar o tópico de Potencia do motor
   		  	SendCmd(cmd); //Envia o comando da assinatura de tópico para o ESP
   		  	while(message == 0);
   		  	message = 0;
   		  	if(UTIL1_strcmp(rcv, "OK SUBSCRIBE\r\n")) { //Comparação da string de resposta do ESP com o OK SUBSCRIBE (0- VERDADEIRO, 1- FALSO)
   		  	  	 SendPC("ERRO NO SUBSCRIBE DO POWER\r\n"); //Escreve no PC que ocorreu erro no subscribe
   		  	  	 count_sub--; //1 dos 4 tópicos para subscribe não aconteceu
   		  	}
   		  	  		 
   		  	strcpy(cmd, "SUBSCRIBE \"EA076/168923/mode\"\r\n"); //Comando para assinar o tópico do Modo de operacao do motor
   		  	SendCmd(cmd); //Envia o comando da assinatura de tópico para o ESP
   		  	while(message == 0);
   		  	message = 0;
   		  	if(UTIL1_strcmp(rcv, "OK SUBSCRIBE\r\n")) { //Comparação da string de resposta do ESP com o OK SUBSCRIBE (0- VERDADEIRO, 1- FALSO)
   		  	  	  SendPC("ERRO NO SUBSCRIBE DO MODE\r\n"); //Escreve no PC que ocorreu erro no subscribe
   		  	  	  count_sub--; //1 dos 4 tópicos para subscribe não aconteceu
   		  	}
   		  	
   		  	strcpy(cmd, "SUBSCRIBE \"EA076/168923/limiar\"\r\n"); //Comando para assinar o tópico do limiar de temperatura
   		    SendCmd(cmd); //Envia o comando da assinatura de tópico para o ESP
   		    while(message == 0);
   		    message = 0;
   		    if(UTIL1_strcmp(rcv, "OK SUBSCRIBE\r\n")) { //Comparação da string de resposta do ESP com o OK SUBSCRIBE (0- VERDADEIRO, 1- FALSO)
   		    	   SendPC("ERRO NO SUBSCRIBE DO LIMIAR\r\n"); //Escreve no PC que ocorreu erro no subscribe
   		    	   count_sub--; //1 dos 4 tópicos para subscribe não aconteceu
   		    }
   		  	  		 
   		  	if (count_sub == 4) {
   		  	  	  SendPC("Concluido\r\n"); //Procedimento de SUBSCRIBE foi concluido com êxito
   		  	}
   		  	count_sub = 4;	
   	   }
  }
  
  A1_SetVal();
  A2_ClrVal();
  
  for(;;) {
	  if(flag_temp_update) { //Se precisa fazer uma nova medicao de temperatura
		  flag_temp_update = 0;
		  Temp_Measure(TRUE); //Faz a medicao
		  Temp_GetValue16(&val); //Armazena o valor convertido do A/D no endereco da variavel val
		  tensao = (3300 * val); 
		  tensao = tensao / 65535; //Valor da tensao em mV
		  temp = tensao - 600; //Temperatura em decimos de graus
		  tempaux = temp; //Armazena o valor original da temperatura na auxiliar
		  
		  strcpy(cmd, "PUBLISH \"EA076/168923/temp\",\""); //Comando de publicar incompleto ainda
		  c = (tempaux / 100) + 0x30; // Construcao da string do valor da temperatura em caracteres ASCII
		  tempaux = tempaux % 100;
		  UTIL1_chcat(cmd, 100, c); //Concatena o caractere c (dezenas, neste caso) ao comando cmd
		  c = (tempaux / 10) + 0x30;
		  UTIL1_chcat(cmd, 100, c);//Concatena as unidades
		  UTIL1_chcat(cmd, 100, '.'); //Concatena o ponto decimal
		  c = (tempaux % 10) + 0x30; //Concatena a casa decimal
		  UTIL1_strcat(cmd, 100, "\"\r\n"); //Concatena o termo de fim de string ao cmd
		  SendCmd(cmd); //Envia o comando de publicacao de temperatura
	  }
	  
	  if(message) { // Se chegou uma mensagem vinda do broker
		  message = 0;
		  if(UTIL1_strFind(rcv, "MESSAGE") == -1) { //Verificar se recebeu uma string contendo MESSAGE
			  SendPC(rcv); //Se não for uma string de MESSAGE, retorna para o PC justamente o que ele recebeu (provavelmente um erro)
		  } else { //Caso seja uma MESSAGE, ele entre nesse else para separar o tópico de mensagem
			  scan = 0;
		    	  		  
		  //Scan de Inicio de Tópico
		      while(rcv[scan] != '['){ //Enquanto o caracter da posição do scan não for o [, o scan é incrementado
		    	  scan++;
		      }
		      scan++; //O scan apontará para a posição logo depois do [ = Primeiro caracter do tópico
		      scan_2 = 0;
		    	  		  
		  //Scan até o Fim do tópico
		      while(rcv[scan] != ']'){ //Enquanto o caracter da posição do scan na mensagem recebida não for o ] :
		    	  top[scan_2] = rcv[scan]; //Coloca na string de tópico o caracter da posição do scan
		    	  scan++; //Incrementa os dois ponteiros scans
		    	  scan_2++;
		      }
		      top[scan_2] = 0; //A String de tópico é finalizada com o 0
		    	  		  
		   //Scan de início do parametro do topico - Mesmo procedimento anterior
		      while(rcv[scan] != '['){
		    	  scan++;
		      }
		      scan++;
		      scan_2 = 0;
		    	  		  
		   //Scan até o fim do parametro do topico - Mesmo procedimento anterior
		      while(rcv[scan] != ']'){
		    	  par[scan_2] = rcv[scan];//Coloca na string de parametro do tópico o caracter da posição do scan
		    	  scan++;
		    	  scan_2++;
		      }
		      par[scan_2] = 0; //A String de parametro do tópico é finalizada com o 0
		  
		    // Interpretacao do comando
		      
		      if(UTIL1_strcmp(top, "EA076/168923/dir") == 0) { //Se o topico for da direcao
		    	  flagdir = 1; //Flag mudanca de direcao
		    	  if(UTIL1_strcmp(par, "Vent") == 0) { //Se o parametro for de ventilador
		    		  dir = 0;
		    	  } else if(UTIL1_strcmp(par, "Exaust") == 0) { //Se o parametro for de exaustor
		    		  dir = 1;
		    	  }
		      }
		      
		      if(UTIL1_strcmp(top, "EA076/168923/power") == 0) { //Se o topico for da potencia
		    	  flagpower = 1; //Flag de mudanca de velocidade
		    	  powertemp = 0;
		    	  i = 0;
		    	  while(par[i]) { //Varredura do topico
		    		  if((par[i] > 0x2F) && (par[i] < 0x3A)) {  //Confere se o caractere eh um numero entre 0 e 9 (valor valido)
		    			 powertemp *= 10; //Construcao do numero da potencia em algarismo por algarismo
		    			 powertemp += (par[i] - 0x30);
		    		  }
		    		  i++;
		    	  }
		    	  if(powertemp < 101) { //Eh um numero valido para potencia (entre 0 e 100)
		    		  power = powertemp;
		    	  }
		      }
		      
		      if(UTIL1_strcmp(top, "EA076/168923/mode") == 0) { //Se o topico for do modo
		    	  flagmode = 1; //Flag mudanca de modo
		    	  if(UTIL1_strcmp(par, "Off") == 0) { //Se o parametro for motor desligado
		    		  mode = 0;
		    	  } else if(UTIL1_strcmp(par, "On") == 0) { //Se o parametro for motor ligado
		    	  	  mode = 1;
		    	  } else if (UTIL1_strcmp(par, "Auto") == 0) { //Se o parametro for modo automatico
		    		  mode = 2;
		    	  }
		      }
		      
		      if(UTIL1_strcmp(top, "EA076/168923/limiar") == 0) { //Se o topico for o limiar de temperatura
		    	  templimiartemp = ((par[0]- 0x30) * 100) + ((par[1] - 0x30) * 10); //Ajusta a centena e dezena da temperatura limiar em decimos de grau
		    	  if(UTIL1_strlen(par) == 4) { // Se o consistir em 4 caracteres (dezena, unidade, ponto e casa decimal)
		    		  templimiartemp += (par[3] - 0x30); //Ajusta a unidade da temperatura em decimos de grau
		    	  }
		    	  //if(templimiartemp < 401) { //O valor de temperatura eh valido sendo ate 40 graus
		    		  templimiar = templimiartemp;
		    	  //}
		      }
		  }  
	  } 
	  
	  if(flagpower){
		  flagpower = 0;
		  if(mode == 1){
			  Power_SetRatio16((100 - power) * 655);
		  }
		  if(mode == 2){
			  if(temp > templimiar){
				  Power_SetRatio16((100 - power) * 655);
			  }
		  }
	  }
	  
	  if(flagdir) { //Se houve mudanca na direcao
		  flagdir = 0;
		  if(dir == 0) { //Sentido Ventilador
			  A1_SetVal();
			  A2_ClrVal();
		  } else { //Sentido Exaustor
			  A1_ClrVal();
			  A2_SetVal();
		  }
	  }
	  
	  if(mode == 0) { // Se esta em modo desligado
		  Power_SetRatio16(0xFFFF); //Desliga o motor
	  } else if(mode == 1) { // Se esta em modo ligado
		  Power_SetRatio16((100 - power) * 655); //Liga o motor na potencia estipulada
	  }
	  
	  if(mode == 2) { //Se esta em modo automatico
		  if(temp > templimiar) {//Checa se a temperatura eh maior que o limiar
			  Power_SetRatio16((100 - power) * 655); //Liga o motor na potencia estipulada
		  } else {
			  Power_SetRatio16(0xFFFF); //Desliga o motor
		  }
	  }
  }
  
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.3 [05.09]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
