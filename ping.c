// Структура ICMP заголовка
struct icmp_header {
    unsigned char  type;        // 8 для Echo Request
    unsigned char  code;        // 0
    unsigned short checksum;    // Контрольная сумма пакета
    unsigned short id;          // Идентификатор
    unsigned short sequence;    // Номер пакета
};

// Функция подсчета контрольной суммы (требуется протоколом IP/ICMP)
unsigned short checksum(unsigned short *addr, int count) {
    long sum = 0;
    while (count > 1) {
        sum += *addr++;
        count -= 2;
    }
    if (count > 0) {
        sum += *(unsigned char *)addr;
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return (unsigned short)(~sum);
}

// Базовый адрес портов RTL8139 (должен совпадать с ядром)
#define IO_BASE 0xC000
#define TSAD0   0x20
#define TSD0    0x10

inline void outl(unsigned short port, unsigned int val) {
    asm volatile ("outl %0, %1" :: "a"(val), "Nd"(port));
}
inline void outw(unsigned short port, unsigned short val) {
    asm volatile ("outw %0, %1" :: "a"(val), "Nd"(port));
}

// Глобальный буфер для отправки пакета
unsigned char packet_buffer[64];

// Точка входа в наш внешний пакет ping
void main() {
    volatile unsigned short* video = (unsigned short*)0xB8000;
    
    // Выводим текст прямо на экран изнутри пакета ping (отступ пониже)
    const char* msg = "-> [PING] Sending ICMP Echo Request to 192.168.1.1...\n";
    int offset = 160 * 10; // 10 строка экрана
    while (*msg) {
        video[offset++] = *msg++ | (0x0B << 8); // Бирюзовый текст
    }

    // Собираем ICMP пакет вручную
    struct icmp_header *icmp = (struct icmp_header *)packet_buffer;
    icmp->type = 8; // Echo Request
    icmp->code = 0;
    icmp->id = 0x1337;
    icmp->sequence = 1;
    icmp->checksum = 0;
    icmp->checksum = checksum((unsigned short *)icmp, sizeof(struct icmp_header));

    // В реальности здесь нужно обернуть ICMP в IP-пакет, 
    // но для теста «голой» сети мы шлем сырой кадр напрямую в чип RTL8139
    
    // Передаем физический адрес буфера сетевой карте для отправки
    outl(IO_BASE + TSAD0, (unsigned int)&packet_buffer[0]);
    
    // Говорим карте отправить пакет (размер пакета 64 байта)
    // Очищаем бит 13 (OWN), чтобы карта поняла: память принадлежит ей
    outl(IO_BASE + TSD0, 64 & 0x1FFF); 
}
