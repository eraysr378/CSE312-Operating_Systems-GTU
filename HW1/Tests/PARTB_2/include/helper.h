
#ifndef __HELPER_H
#define __HELPER_H

#include <common/types.h>

namespace myos
{
    void printf(char *str);

    void printfHex(common::uint8_t key);

    void printfHex16(common::uint16_t key);

    void printfHex32(common::uint32_t key);

    void printInteger(int number);

    int atoi(char *str);
    char *itoa(int number);
    void clearScreen();
    class CanRun
    {
    public:
        static bool canRun; // Declaration
    };

}

#endif