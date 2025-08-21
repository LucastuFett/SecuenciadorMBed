#include "mbed.h"

BufferedSerial uart(p4, p5);
uint8_t buffer[6] = {0x90, 0x45, 0x7F, 0x80, 0x45, 0x00}; // Example MIDI message

int main(){
    uart.set_baud(31250); // Set baud rate for MIDI communication
    uart.set_format(8, BufferedSerial::None, 1); // 8 data
    while(1){
        printf("Enviando datos\n");
        uart.write(buffer, sizeof(buffer)); // Enviar mensaje MIDI
        ThisThread::sleep_for(1000ms); // Esperar 1 segundo antes de enviar de nuevo
    }
}