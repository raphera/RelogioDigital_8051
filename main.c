/*
 * Ultima Atualização-------------------
 * Data: 30/06/2019
 * Hora: 18h29m
 * Por: Raphael
 * Implementado:
 *  - Melhoria dos comentários no código;
 *  - Implementada função para mostrar
 *    data e ano sem alterar dados salvos.
 * */

#include <8051.h>

/* Configurações personalizadas do programa */
#define MAX_ALARMS 10
#define DELAY_WR_DISP 1 // Kit -> 5 | Proteus -> 1

/* Endereços das protas externas */
#define DISP7S 0xFFC0
#define DISP7S_SLT 0xFFC1
#define KEYBOARD 0xFFC3
#define DAC 0xFFE4
#define ADC 0xFFE5
#define MOTOR 0xFFE6
#define DISPLAY_IR 0xFFC2
#define DISPLAY_DR 0xFFD2
#define DISPLAY_BF 0xFFE2
#define DISPLAY_R 0xFFF2

// Valores retornados pelas teclas do teclado
// Linha 1
#define T1 0x0001 // AD6 & ACH10 - CH2
#define T2 0x0002 // AD6 & ACH11 - CH3
#define T3 0x0004 // AD6 & ACH12 - CH4
#define TA 0x0008 // AD6 & ACH13 - CH5
// Linha 2
#define T4 0x0010 // AD6 & ACH14 - CH6
#define T5 0x0020 // AD6 & ACH15 - CH7
#define T6 0x0040 // AD6 & ACH16 - CH8
#define TB 0x0080 // AD6 & ACH17 - CH9
// Linha 3
#define T7 0x0100 // AD7 & ACH10 - CH10
#define T8 0x0200 // AD7 & ACH11 - CH11
#define T9 0x0400 // AD7 & ACH12 - CH12
#define TC 0x0800 // AD7 & ACH13 - CH13
// Linha 4
#define TF 0x1000 // AD7 & ACH14 - CH14
#define T0 0x2000 // AD7 & ACH15 - CH15
#define TE 0x4000 // AD7 & ACH16 - CH16
#define TD 0x8000 // AD7 & ACH17 - CH17

/* Variáveis de controle do Buzzer */
	__xdata unsigned int tmp;
	unsigned char __far __at DAC amost;

/* Variável utilizada para fazer o controle
  // de quantas vezes a função Timer0_ISR: foi
  chamada; */
unsigned int usecCounter = 0;

/* Vetor contendo os bits (em hex) para
  acender os segmentos de acordo com o
  caractere (atrelado ao indice do
  vetor).*/
__code unsigned char num_sete_seg[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7C, // 6
    0x07, // 7
    0x7F, // 8
    0x67, // 9
    0x77, // A
    0x40  // -
};

/* Vetor com o endereço de cada display
  de 7 segmentos. */
__code unsigned char num_disp[] = {
    0x00, // Valor para adequar a funcao
    0x08, // Display 1
    0x04, // Display 2
    0x02, // Display 3
    0x01  // Display 4
};

/* Struct com dados básicos do Relógio */
struct Relogio {
  unsigned char Hora;
  unsigned char Minuto;
  unsigned char Segundo;
  unsigned int MSegundo;
  unsigned char Dia;
  unsigned char Mes;
  unsigned int Ano;
};

__xdata struct Relogio
    Def; // Struct definida apenas para trabalhar com função de definição
struct Relogio
    Atual; // Strcuct utilizada apenas para o mostrador (mostra os dados atuais)
__xdata struct Relogio
    Alarme[MAX_ALARMS]; // Vetor de Structs contendo todos os alarmes.

/* Struct Criada para trabalhar junto com a função Borda_Subida(),
  pois cada botão precisa de sua variável Aux. */
struct AuxB {
  unsigned char B0;
  unsigned char B1;
  unsigned char B2;
  unsigned char B3;
  unsigned char B4;
  unsigned char B5;
  unsigned char B6;
  unsigned char B7;
  unsigned char B8;
  unsigned char B9;
  unsigned char BA;
  unsigned char BB;
  unsigned char BC;
  unsigned char BD;
  unsigned char BE;
  unsigned char BF;
};

struct AuxB AuxB;

__bit Pisca500ms;

// Vetor que que define cada alarme como ativo ou inativo
unsigned char Alarme_en[MAX_ALARMS];
// Variável que define se há pelo menos um alarme tocando no momento
signed char Alarme_tocando;

// Vetor para definir cada display se está aceso ou apagado
unsigned char Ajuste[5];

/* Conjunto de variáveis do tipo __bit que definem aperto de um botão */
__bit Tecla0;
__bit Tecla1;
__bit Tecla2;
__bit Tecla3;
__bit Tecla4;
__bit Tecla5;
__bit Tecla6;
__bit Tecla7;
__bit Tecla8;
__bit Tecla9;
__bit TeclaA;
__bit TeclaB;
__bit TeclaC;
__bit TeclaD;
__bit TeclaE;
__bit TeclaF;

/* Função implementada para atraso de máquina de acordo com argumento
  em ms (milissegundos) passado ao chamar função. */
void Delay(unsigned int Tempo) {
  unsigned int x;
  while (Tempo-- > 0) {
    for (x = 0; x <= 150; x++)
      ;
  }
}

/* Função que verifica que ano é bissexto retornando 1 para bissexto
  ou 0 para não bissexto. */
unsigned char Bissexto(unsigned char Ano) {
  unsigned char Resultado;
  if (((Ano % 4 == 0) && (Ano % 100 != 0)) ||
      ((Ano % 4 == 0) && (Ano % 100 == 0) && (Ano % 400 == 0))) {
    Resultado = 1;
  } else {
    Resultado = 0;
  }
  return Resultado;
}

/* Verifica a quantidade de dias que o mês possui de acordo com argumentos Ano
  e Mês passados no argumento. */
unsigned char DiaMax(unsigned char Mes, unsigned char Ano) {
  unsigned char Resultado;
  if (Mes == 4 | Mes == 6 | Mes == 9 | Mes == 11) {
    Resultado = 30;
  } else if ((Mes == 2) & (Bissexto(Ano))) {
    Resultado = 29;
  } else if ((Mes == 2) & !(Bissexto(Ano))) {
    Resultado = 28;
  } else {
    Resultado = 31;
  }
  return Resultado;
}

/* Faz varredura no vetor de alarmes (Alarme[MAX_ALARMS]) procurando por alarme
  compatível com tempo atual, caso tenha algum, seta variável alarme tocando com
  indice do vetor que corresponde ao alarme + 1 (ajuste de indice para
  utilização no display). */
void alarme_verif() {
  Alarme_tocando = 0;
  for (char i = 0; i < MAX_ALARMS; i++) {
    if (Alarme_en[i])
      if (Alarme[i].Ano == Atual.Ano)
        if (Alarme[i].Mes == Atual.Mes)
          if (Alarme[i].Dia == Atual.Dia)
            if (Alarme[i].Hora == Atual.Hora)
              if (Alarme[i].Minuto == Atual.Minuto) {
                Alarme_tocando = i + 1;
                break;
              }
  }
}

/* Função chamada a cada ciclo da interrupção e cuida de fazer a atuliazação
  da Struct Atual, Struct essa que armazena os valores de tempo atuais. */
void Relogio_upd(void) {
  if (Atual.MSegundo < 999) {
    Atual.MSegundo++;
  } else {
    Atual.MSegundo = 0;
    if (Atual.Segundo < 59) {
      Atual.Segundo++;
    } else {
      Atual.Segundo = 0;
      if (Atual.Minuto < 59) {
        Atual.Minuto++;
      } else {
        Atual.Minuto = 0;
        if (Atual.Hora < 23) {
          Atual.Hora++;
        } else {
          Atual.Hora = 0;
          if (Atual.Dia < DiaMax(Atual.Mes, Atual.Ano)) {
            Atual.Dia++;
          } else {
            Atual.Dia = 1;
            if (Atual.Mes < 12) {
              Atual.Mes++;
            } else {
              Atual.Mes = 1;
              Atual.Ano++;
            }
          }
        }
      }
    }
  }
}

/* Função utilizada para controle dos displays de 7 segmentos, acende
  os segmentos do display e/ou ponto de acordo com os argumentos passados. */
void Escreve7Seg(__bit Liga, __bit Ponto, unsigned char Posicao,
                 unsigned char Digito) {
  static unsigned char __far __at DISP7S D7Seg;
  static unsigned char __far __at DISP7S_SLT Saida;

  Saida = num_disp[Posicao];
  // Nao acende o digito 0 na casa mais a esquerda
  // Liga = Digito == 0 && Posicao == 4 ? 0 : Liga;
  D7Seg = (num_sete_seg[Digito] | (0x80 * Ponto)) & (0xFF * Liga);
}

/* Função que define as opções o timer do microcontrolador */
void InitTimer(void) {

  TMOD = 0b00000010; // Setar Timer0 em modo 2 (recarrega valor inicial)
  PT0 = 1;    //Timer0 com prioridade alta
  TH0 = 6; // Tempo de recarregar de 250us
  TL0 = 6; // Valor de inicio

  ET0 = 1; // Ativa as interrupcoes do Timer 0
  EA = 1;  // Ativa a interrupcao global

  TR0 = 1; // Inicia o Timer 0
}

/* Função chamada pela interrupção do microcontrolador de acordo com
  as opções definidas na função InitTimer0. */
void Timer0_ISR(void) __interrupt 1 {
  TF0 = 0;              // Limpa o flag de interrupcao
  usecCounter++;        // Proporcao de 1 - 250us
  if (usecCounter == 4) // 1000us == 1ms
  {
    Relogio_upd(); // Chama a funcao que atualiza o tempo
    usecCounter = 0;
  }
}

/* Função faz varredura nos buffers do teclado procurando por teclas que
  foram pressionadas e retorna interiro com resultado. */
unsigned int Tecla(void) {
  static unsigned char __far __at KEYBOARD Teclado;
  unsigned char T123A456B;
  unsigned char T789CF0ED;
  unsigned int Resultado = 0x00;

  Teclado = 0b10000000;
  T123A456B = Teclado;
  Teclado = 0b01000000;
  T789CF0ED = Teclado;

  Resultado |= T123A456B;
  Resultado = Resultado << 8;
  Resultado |= T789CF0ED;

  return Resultado;
}

/* Detecta borda de subida no sinal de entrada, função utilizada
  variável única para cada sinal a ser definida no argumento junto
  com a entrada do sinal. */
__bit Borda_Subida(__bit Entrada, unsigned char *Aux) {
  __bit Borda = 0;
  if (Entrada != (*Aux)) {
    if (Entrada) {
      Borda = 1;
      (*Aux) = Entrada;
    } else { // Correcao da verificacao de borda - Raphael (21/06/19)
      (*Aux) = Entrada;
    }
  } else {
    Borda = 0;
  }
  return Borda;
}

/* Função que decompõe valor retornado pela função Tecla() e nos respectivos
  bits das teclas (Tela0, Tecla1, ..., TeclaF) e também retorna qual tecla foi
  pressionada (apenas uma com prioridade TeclaF > Tecla0). */
signed char verif_teclas() {
  signed char retorno = -1;

  retorno = (Tecla0 = T0 == (Tecla() & T0)) == 1 ? 0 : retorno;
  retorno = (Tecla1 = T1 == (Tecla() & T1)) == 1 ? 1 : retorno;
  retorno = (Tecla2 = T2 == (Tecla() & T2)) == 1 ? 2 : retorno;
  retorno = (Tecla3 = T3 == (Tecla() & T3)) == 1 ? 3 : retorno;
  retorno = (Tecla4 = T4 == (Tecla() & T4)) == 1 ? 4 : retorno;
  retorno = (Tecla5 = T5 == (Tecla() & T5)) == 1 ? 5 : retorno;
  retorno = (Tecla6 = T6 == (Tecla() & T6)) == 1 ? 6 : retorno;
  retorno = (Tecla7 = T7 == (Tecla() & T7)) == 1 ? 7 : retorno;
  retorno = (Tecla8 = T8 == (Tecla() & T8)) == 1 ? 8 : retorno;
  retorno = (Tecla9 = T9 == (Tecla() & T9)) == 1 ? 9 : retorno;
  retorno = (TeclaA = TA == (Tecla() & TA)) == 1 ? 10 : retorno;
  retorno = (TeclaB = TB == (Tecla() & TB)) == 1 ? 11 : retorno;
  retorno = (TeclaC = TC == (Tecla() & TC)) == 1 ? 12 : retorno;
  retorno = (TeclaD = TD == (Tecla() & TD)) == 1 ? 13 : retorno;
  retorno = (TeclaE = TE == (Tecla() & TE)) == 1 ? 14 : retorno;
  retorno = (TeclaF = TF == (Tecla() & TF)) == 1 ? 15 : retorno;

  return retorno;
}

/* Função faz a escrita da hora no display passando a Struct desejada
  no argumento. */
void escreve_hora(struct Relogio *mostrar) {
  Escreve7Seg(Ajuste[1], 0, 1, mostrar->Minuto % 10);
  Delay(DELAY_WR_DISP);
  Escreve7Seg(Ajuste[2], 0, 2, mostrar->Minuto / 10);
  Delay(DELAY_WR_DISP);
  Escreve7Seg(Ajuste[3], Pisca500ms, 3, mostrar->Hora % 10);
  Delay(DELAY_WR_DISP);
  Escreve7Seg(Ajuste[4], 0, 4, mostrar->Hora / 10);
  Delay(DELAY_WR_DISP);
}

/* Função faz a escrita da data no display passando a Struct desejada
  no argumento e definindo argumento ano como 0, e escreve ano ao
  definir atumento ano como 1. */
void escreve_data(struct Relogio *mostrar, __bit ano) {
  if (ano) {
    Escreve7Seg(Ajuste[1], 0, 1, (((mostrar->Ano) % 1000) % 100) % 10);
    Delay(DELAY_WR_DISP);
    Escreve7Seg(Ajuste[2], 0, 2, (((mostrar->Ano) % 1000) % 100) / 10);
    Delay(DELAY_WR_DISP);
    Escreve7Seg(Ajuste[3], 0, 3, (((mostrar->Ano) % 1000) / 100));
    Delay(DELAY_WR_DISP);
    Escreve7Seg(Ajuste[4], 0, 4, (mostrar->Ano) / 1000);
    Delay(DELAY_WR_DISP);
  } else {
    Escreve7Seg(Ajuste[1], 0, 1, mostrar->Mes % 10);
    Delay(DELAY_WR_DISP);
    Escreve7Seg(Ajuste[2], 0, 2, mostrar->Mes / 10);
    Delay(DELAY_WR_DISP);
    Escreve7Seg(Ajuste[3], 1, 3, mostrar->Dia % 10);
    Delay(DELAY_WR_DISP);
    Escreve7Seg(Ajuste[4], 0, 4, mostrar->Dia / 10);
    Delay(DELAY_WR_DISP);
  }
}

/* Função que define Data(Mes e Ano) e Hora de uma Struct qualquer,
 * podendo ser Alarme[1] ou Atual por exemplo.
 * __bit -> 1 : Seta configs definidas na funcao.
 * __bit -> 2 : Copia informacoes da struct passada no
 *  argumento para mostrar durante as alteracoes.
 * */
void def(unsigned char inicial, struct Relogio *final) {
  if (inicial == 1) {
    Def.Dia = 21;
    Def.Mes = 06;
    Def.Ano = 2019;
    Def.Hora = 12;
    Def.Minuto = 0;
    Def.Segundo = 0;
    Def.MSegundo = 0;
  } else if (inicial == 2) {
    Def = (*final);
  }

  unsigned char ponteiro = 4;
  signed char tecla_ap = -1;
  Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

  // ------Altera o Ano------
  // Criado Array para facilitar a manipulacao individual
  unsigned char Ano[5];
  Ano[4] = (Def.Ano) / 1000;
  Ano[3] = ((Def.Ano) % 1000) / 100;
  Ano[2] = (((Def.Ano) % 1000) % 100) / 10;
  Ano[1] = (((Def.Ano) % 1000) % 100) % 10;
  while (!Borda_Subida(TeclaD, &AuxB.BD)) {

    // Altera posicao do ponteiro (digito a ser alterado)
    ponteiro =
        Borda_Subida(TeclaF, &AuxB.BF) & (ponteiro < 4) ? ++ponteiro : ponteiro;
    ponteiro =
        Borda_Subida(TeclaE, &AuxB.BE) & (ponteiro > 1) ? --ponteiro : ponteiro;

    Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

    // Pisca digito onde esta o ponteiro
    Ajuste[ponteiro] = Atual.MSegundo > 500 ? 1 : 0;

    // Altera dado
    if (tecla_ap >= 0) {
      // Verifica se foi apertado um botao de 0 a 9 e salva valor no digito do
      // ponteiro
      Ano[ponteiro] = tecla_ap < 10 ? tecla_ap : Ano[ponteiro];
      // Converte o Array para inteiro e salva no elemento Ano da struct Def
      Def.Ano = Ano[4] * 1000 + Ano[3] * 100 + Ano[2] * 10 + Ano[1];
      if (Ano[4] == 0 & Ano[3] == 0 & Ano[2] == 0)
        if (Ano[1] < 1)
          Ano[1] = 1;
    }

    // Escreve no Display
    escreve_data(&Def, 1);

    // Faz varredura nas teclas
    tecla_ap = verif_teclas();
  }
  //------Fim de alteração do ano------

  // Reset das variáveis
  ponteiro = 4;
  tecla_ap = -1;
  Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

  // -------Altera Dia/Mes-------
  // Criado Array para facilitar a manipulacao individual
  unsigned char DiaMes[5];
  DiaMes[4] = Def.Dia / 10;
  DiaMes[3] = Def.Dia % 10;
  DiaMes[2] = Def.Mes / 10;
  DiaMes[1] = Def.Mes % 10;
  unsigned char Last_D = 0;
  while (!Borda_Subida(TeclaD, &AuxB.BD)) {
    // Altera posicao do ponteiro (digito a ser alterado)
    ponteiro =
        Borda_Subida(TeclaF, &AuxB.BF) & (ponteiro < 4) ? ++ponteiro : ponteiro;
    ponteiro =
        Borda_Subida(TeclaE, &AuxB.BE) & (ponteiro > 1) ? --ponteiro : ponteiro;

    Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

    // Pisca digito onde esta o ponteiro
    Ajuste[ponteiro] = Atual.MSegundo > 500 ? 1 : 0;

    // Define os qtd de dias Max do mes
    Last_D = DiaMax(Def.Mes, Def.Ano);

    // Altera dado
    if (tecla_ap >= 0) {
      switch (ponteiro) {
        // Cases do Mes
      case 1:
        if (DiaMes[2] == 1)
          DiaMes[1] = tecla_ap < 3 ? tecla_ap : 2;
        else
          DiaMes[1] = tecla_ap < 10 ? tecla_ap : DiaMes[ponteiro];
        break;

      case 2:
        DiaMes[2] = tecla_ap < 2 ? tecla_ap : DiaMes[ponteiro];
        if (DiaMes[2] == 1)
          DiaMes[1] = DiaMes[1] > 2 ? 2 : DiaMes[1];
        else
          DiaMes[1] = DiaMes[1] < 1 ? 1 : DiaMes[1];
        break;

        // Cases do Dia
      case 3:
        DiaMes[3] = tecla_ap < 10 ? tecla_ap : DiaMes[3];
        if (DiaMes[4] == Last_D / 10)
          if (DiaMes[3] > Last_D % 10)
            DiaMes[4] = 0;

        break;

      case 4:
        DiaMes[4] = tecla_ap <= (Last_D / 10) ? tecla_ap : DiaMes[4];
        if (DiaMes[4] == Last_D / 10)
          DiaMes[3] = DiaMes[3] > (Last_D % 10) ? Last_D % 10 : DiaMes[3];
        else if (DiaMes[4] == 0)
          DiaMes[3] = DiaMes[3] > 0 ? DiaMes[3] : 1;

        break;

      default:
        break;
      }

      Def.Dia = DiaMes[4] * 10 + DiaMes[3];
      Def.Mes = DiaMes[2] * 10 + DiaMes[1];
    }

    // Escreve no Display
    escreve_data(&Def, 0);

    // Faz varredura nas teclas
    tecla_ap = verif_teclas();
  }
  //------Fim de alteração de Dia/Mes------

  // Reset das variáveis
  ponteiro = 4;
  tecla_ap = -1;
  Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

  // ------Altera Hora------
  // Criado Array para facilitar a manipulacao individual
  unsigned char Hora[5];
  Hora[4] = Def.Hora / 10;
  Hora[3] = Def.Hora % 10;
  Hora[2] = Def.Minuto / 10;
  Hora[1] = Def.Minuto % 10;
  while (!Borda_Subida(TeclaD, &AuxB.BD)) {
    // Altera posicao do ponteiro (digito a ser alterado)
    ponteiro =
        Borda_Subida(TeclaF, &AuxB.BF) & (ponteiro < 4) ? ++ponteiro : ponteiro;
    ponteiro =
        Borda_Subida(TeclaE, &AuxB.BE) & (ponteiro > 1) ? --ponteiro : ponteiro;

    Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

    // Pisca digito onde esta o ponteiro
    Ajuste[ponteiro] = Atual.MSegundo > 500 ? 1 : 0;

    // Altera dado
    if (tecla_ap >= 0) {
      switch (ponteiro) {
        // Cases dos Minutos
      case 1:
        Hora[1] = tecla_ap < 10 ? tecla_ap : Hora[1];
        break;

      case 2:
        Hora[2] = tecla_ap < 6 ? tecla_ap : Hora[2];
        break;

        // Cases das Horas
      case 3:
        Hora[3] = tecla_ap < 10 ? tecla_ap : Hora[3];
        if (Hora[3] > 3 && Hora[4] > 1)
          Hora[4] = 0;

        break;

      case 4:
        Hora[4] = tecla_ap < 3 ? tecla_ap : Hora[4];
        if (Hora[4] == 2)
          Hora[3] = Hora[3] > 3 ? 3 : Hora[3];
        else if (Hora[4] == 0)
          Hora[3] = Hora[3] > 0 ? Hora[3] : 1;

        break;

      default:
        break;
      }

      Def.Hora = Hora[4] * 10 + Hora[3];
      Def.Minuto = Hora[2] * 10 + Hora[1];
    }

    // Escreve no Display
    escreve_hora(&Def);

    // Faz varredura nas teclas
    tecla_ap = verif_teclas();
  }
  //------Fim de alteração de Hora------

  // Salva alterações na Struct passada como argumento
  (*final) = Def;
}

/* Função faz a escrita do menu de alarme no display passando os
  argumentos de indice e ativo. */
void menu_alarme(unsigned char alarme, __bit ativo) {
  Escreve7Seg(Ajuste[1], ativo, 1, alarme % 10);
  Delay(DELAY_WR_DISP);
  Escreve7Seg(Ajuste[2], ativo, 2, alarme / 10);
  Delay(DELAY_WR_DISP);
  Escreve7Seg(Ajuste[3], 0, 3, 11);
  Delay(DELAY_WR_DISP);
  Escreve7Seg(Ajuste[4], 0, 4, 10);
  Delay(DELAY_WR_DISP);
}

/* Função que gera e faz o controle do menu dos alarmes */
void alarme() {
  signed char tecla_ap = -1;
  unsigned char alarm_at = 1;
  // Selecao de alarme com o menu
  while (!Borda_Subida(TeclaD, &AuxB.BD)) {

    if (Borda_Subida(TeclaE, &AuxB.BE) & (alarm_at < MAX_ALARMS))
      alarm_at++;

    if (Borda_Subida(TeclaF, &AuxB.BF) & (alarm_at > 1))
      alarm_at--;

    if (Borda_Subida(Tecla0, &AuxB.B0))
      Alarme_en[alarm_at - 1] = Alarme_en[alarm_at - 1] == 1 ? 0 : 1;

    Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

    // Altera dado
    if (tecla_ap >= 0) {
    }

    // Escreve no Display
    menu_alarme(alarm_at, Alarme_en[alarm_at - 1]);

    // Faz varredura nas teclas
    tecla_ap = verif_teclas();
  }

  def(2, &Alarme[alarm_at - 1]);
}

/* Função para mostrar a data no Display, ao ser chamada mostra
  primeiramente o ano, apos pressionar a teclaC mostra dia e mês
  pressionando mais uma vez sai da função. */
void data(struct Relogio *atual) {
  // Ativa todos os 4 displays
  Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

  // ------Ano------
  while (!Borda_Subida(TeclaC, &AuxB.BC)) {
    // Escreve no Display
    escreve_data(atual, 1);

    // Faz varredura nas teclas apertadas
    verif_teclas();
  }
  //------Fim do ano------

  // -------Dia/Mes-------
  while (!Borda_Subida(TeclaC, &AuxB.BC)) {
    // Escreve no Display
    escreve_data(atual, 0);

    // Faz varredura nas teclas apertadas
    verif_teclas();
  }
  //------Dia/Mes------
}

/* Função que emite sinal para buzzer */
void Alarme_buzzer(){
	if(Atual.MSegundo > 500){
		for (tmp = 0; tmp < 6; tmp++)
			amost = 0;
		for (tmp = 6; tmp > 0; tmp--)
			amost = 255;	
	}
}

void main(void) {

  /* Configura os pinos de entrada e saída (botão e LED) da P1
    de acordo com o Kit */
  P1 = 0xF0;

  /* Inicializa o Timer para acionar interrupcao */
  InitTimer();

  /* Chama a função para definir inicialmente o tempo */
  def(1, &Atual);

  /* Seta todos os alarmes inicialmente como desativados (0 em todos os indices
    do Alarme_en), e copia a data inicial definida para facilitar na hora de
    configurara cada alarme. */
  for (char i = 0; i < MAX_ALARMS; i++) {
    Alarme[i] = Atual;
    Alarme_en[i] = 0;
  }

  while (1) {

    /* Verifica alarmes a cada minuto */
    alarme_verif();

    /* Variável que define tempo que ponto no display entre hora e minuto pisca.
     */
    Pisca500ms = Atual.MSegundo > 500 ? 1 : 0;

    /* Verifica se tem algum alarme tocando, se tiver pisca display */
    if (Alarme_tocando) {
      Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = Pisca500ms;
	Alarme_buzzer();
      if (Tecla0)
        Alarme_en[Alarme_tocando - 1] = 0;
    } else
      Ajuste[1] = Ajuste[2] = Ajuste[3] = Ajuste[4] = 1;

    /* Chama funcao que faz varredura nas teclas */
    verif_teclas();

    /* Verifica o acionamento das teclas para chamar as funções destinadas a
     * cada uma */
    if (Borda_Subida(TeclaA, &AuxB.BA))
      alarme();
    else if (Borda_Subida(TeclaB, &AuxB.BB))
      def(2, &Atual);
    else if (Borda_Subida(TeclaC, &AuxB.BC))
      data(&Atual);
    else
      escreve_hora(&Atual);
  }
}
