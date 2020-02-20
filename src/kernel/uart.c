#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <common/stdlib.h>

void mmio_write(uint32_t reg, uint32_t data)
{
    *(volatile uint32_t*)reg = data;
}

uint32_t mmio_read(uint32_t reg)
{
    return *(volatile uint32_t*)reg;
}

// Loop <delay> times in a way that the compiler won't optimize away
void delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
            : "=r"(count): [count]"0"(count) : "cc");
}

void uart_init()
{
    mmio_write(UART0_CR, 0x00000000);               //Zera o registro de controle do UART

    mmio_write(GPPUD, 0x00000000);                  //Escreve zero no GPPUD
    delay(150);

    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));   //Escreve no GPPUDCLK0 marcando quais pinos devem ser desativados
    delay(150);

    mmio_write(GPPUDCLK0, 0x00000000);              //Grava zero no GPPUDCLK0 para que tudo funcione

    mmio_write(UART0_ICR, 0x7FF);                   //Limpa todas as interrupções pendentes do hardware UART

    mmio_write(UART0_IBRD, 1);
    mmio_write(UART0_FBRD, 40);                     //Obtendo uma taxa de transmissão de 115200 bits/s

    /*
    Grava os bits 4, 5 e 6 no registro de controle de linha
    A configuração do bit 4 significa que o hardware UART manterá os dados em um FIFO com profundidade de 8 itens, em vez de um registro com profundidade de 1 item
    Definir 5 e 6 como 1 significa que os dados enviados ou recebidos terão palavras longas de 8 bits.
    */
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    //Isso desativa todas as interrupções do UART escrevendo um em todos os bits relevantes do registro Interrupt Mask Set Clear.
    mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | 
                           (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    //Isso grava os bits 0, 8 e 9 no registro de controle. O bit 0 habilita o hardware UART, o bit 8 habilita a capacidade de receber dados e o bit 9 habilita a capacidade de transmitir dados.
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char c)
{
    //Checa se o FIFO de gravação pode aceitar qualquer dado
    while ( mmio_read(UART0_FR) & (1 << 5) ) { }
    mmio_write(UART0_DR, c);
}

unsigned char uart_getc()
{
    //Checa se o FIFO de leitura tem algum dado para ser lido
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}

void uart_puts(const char* str)
{
    //Para cada caracter da string
    for (size_t i = 0; str[i] != '\0'; i ++)
        //Escreve o caracter no UART
        uart_putc((unsigned char)str[i]);
}