#include <mbed.h>
#include "char_search.h"

bool is_char_symbol(char c)
{
    if (c == '-' || c == '+' || c == '/' || c == '*' || c == '^' || c == '(' || c == ')')
        return true;
    else
        return false;
}

bool is_char_free_symbol(char c)
{
    if (c == '-' || c == '+')
        return true;
    else
        return false;
}