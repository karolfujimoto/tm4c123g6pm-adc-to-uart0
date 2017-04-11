#include "tm4c123gh6pm.h"
#include <inttypes.h>
#include <math.h>
/*
 * TODO: Leitura análogica de temperatura
 * TODO: Leitura analógica de tensão do motor
 * TODO: Envio de dados via UART0
 * TODO: Conferir valores
 * main.c
 */
unsigned char charInput(void);
void uart_setup();
void tx_char(unsigned char c);
void setup_adc0(void);
uint16_t get_temp(void);
uint16_t number_length(uint16_t number);
char * int_to_char(uint16_t value, uint16_t length) {
	uint16_t base_one, base_two, i, j;
	char str[10];

	j = 0;
	for(i = length; i > 0;i--) {
		base_one = (uint16_t) pow(10,i);
		base_two = (uint16_t) pow(10,i-1);
		str[j] = '0' + (value%base_one)/base_two;
		j++;
	}
	return str;
}
int main(void) {
	uint16_t temp, length, i;
	char *str;
	uart_setup();
	setup_adc0();
	while(1) {
		temp = get_temp();
		length = number_length(temp);
		if(length) {
			str = int_to_char(temp,length);
			for(i = 0;i < length;i++) {
				tx_char(*(str+i));
			}
		}
		else
			tx_char('0');
		tx_char('\r');
		tx_char('\n');
	}
	return 0;
}
void setup_adc0(void) {
	//Configure AD converter
	//Run Mode Gating Control Register 2 - Enable clock to GPIOE - Pag. 464
	SYSCTL_RCGC2_R |= 0x00000010;
	//GPIO PortE Data Direction - Input - Pag.
	GPIO_PORTE_DIR_R &= ~0x08;
	//GPIO Alternative Funcion Selection - Enable PORTE3 alternative function - Pag. 672
	GPIO_PORTE_AFSEL_R |= 0x08;
	//GPIO Digital Enable - Disable digital function on PE3 - Pag.
	GPIO_PORTE_DEN_R &= ~0x08;
	//GPIO Analog Mode Select - Disable isolation do make analog function work - Pag. 687
	GPIO_PORTE_AMSEL_R |= 0x08;
	//Run Mode Gating Control Register 2 - Enable clock to GPIOE - Pag. 464
	SYSCTL_RCGC2_R |= 0x00000010;
	//Run Mode Gating Control Register 0 - Enable ADC0 - Pag. 457
	SYSCTL_RCGC0_R |= 1 << 16;
	//Run Mode Gating Control Register 0 - Configure to 125k sampling rate - Pag. 457
	SYSCTL_RCGC0_R &= ~0x00000300;
	//ADC Sample Sequence Priority - Set Sequencer 3 to High Priority - Pag. 841
	ADC0_SSPRI_R = 0x0123;
	//ADC Active Sample Sequencer - Disable Sample Sequencer 3 - Pag. 821
	ADC0_ACTSS_R &= ~0x08;
	//ADC Event Multiplexer Select - Enabled by software - Pag.833
	ADC0_EMUX_R &= ~0xF000;
	//ADC Sample Sequence Input Multiplexer Select 3 - Set Sample Sequencer to PE3 (Ain0) - Pag. 875
	ADC0_SSMUX3_R = ~0x000F;
	//ADC Sample Sequence Control - End0 Register - Pag. 859
	ADC0_SSCTL3_R = 0x0006;
	//ADC Active Sample Sequencer - Enable Sample Sequencer 3 - Pag. 821
	ADC0_ACTSS_R |= 0x08;

}
void uart_setup() {
	SYSCTL_RCGC1_R |= 1;
	SYSCTL_RCGC2_R |= 1;
	SYSCTL_RCGCUART_R |= 1;
	SYSCTL_RCGCGPIO_R |= 1;

	UART0_CTL_R &= ~0x1;		//disable uart0
	UART0_IBRD_R=104;		     //(clock cpu = 16mhz), (16x10^6/(9600*16))=104 @9600
	UART0_FBRD_R=11;			//(16x10^6/(9600*16))-104 = resto * 64 => 11
	UART0_LCRH_R |= (UART_LCRH_WLEN_8|UART_LCRH_FEN); //((1<<6)|(1<<5)|(1<<4)); //8 bit FIFO enable
	UART0_CTL_R|= 0x1;			//enable uart0

	GPIO_PORTA_AFSEL_R |= 0x3;
	GPIO_PORTA_DEN_R |= 0x3;
	GPIO_PORTA_PCTL_R=((GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011);
	GPIO_PORTA_AMSEL_R &= ~0x3;
}
uint16_t get_temp(void){
	uint16_t result;
	ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
	while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
	result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
	ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
	return result;
}
void tx_char(unsigned char c) {
	while(UART0_FR_R&UART_FR_TXFF) {};
	UART0_DR_R = c;
}
uint16_t number_length(uint16_t number) {
	uint16_t size = 0;
	while(number) {
		number /= 10;
		size++;
	}
	return size;
}
