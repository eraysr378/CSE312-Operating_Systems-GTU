#include <helper.h>
using namespace myos;
using namespace myos::common;

static uint8_t x = 0, y = 0;
bool CanRun::canRun = false;
void myos::printf(char *str)
{
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;


    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}
void myos::clearScreen(){
    while(x!=0 || y!= 0){
        printf(" ");
    }

}


void myos::printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void myos::printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}
void myos::printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}

void myos::printInteger(int num)
{

    char buffer[50];
    char tmpBuffer[50];
    int digits = 0;
    int index = 0;
    int start = 0;
    bool isNegative = num < 0;

    if (isNegative)
    {
        num *= -1;
        // if number is negative first index will be "-" sign
        index = 1;
        start = 1;
    }

    do
    {
        buffer[index] = '0' + (num % 10);
        num /= 10;
        index++;
    } while (num > 0);

    for (int i = 0; i < index; i++)
    {
        tmpBuffer[i + start] = buffer[index - i - 1];
    }
    if (isNegative)
    {
        tmpBuffer[0] = '-';
    }
    tmpBuffer[index] = '\0';

    printf(tmpBuffer);
}

int myos::atoi(char *str)
{
    bool isNegative = str[0] == '-';
    int i = isNegative ? 1 : 0;
    int num = 0;
    for (; str[i] != '\0'; i++)
    {
        num *= 10;
        num += str[i] - '0';
    }
    if (isNegative)
        num *= -1;
    return num;
}
char* myos::itoa(int num) {
    char buffer[50];
    char *tmpBuffer = new char[50]; // Dynamically allocate memory

    int digits = 0;
    int index = 0;
    int start = 0;
    bool isNegative = num < 0;

    if (isNegative) {
        num *= -1;
        // if number is negative first index will be "-" sign
        index = 1;
        start = 1;
    }

    do {
        buffer[index] = '0' + (num % 10);
        num /= 10;
        index++;
    } while (num > 0);

    for (int i = 0; i < index; i++) {
        tmpBuffer[i + start] = buffer[index - i - 1];
    }
    if (isNegative) {
        tmpBuffer[0] = '-';
    }
    tmpBuffer[index] = '\0';
    return tmpBuffer;
}
