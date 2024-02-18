#ifndef PRINT_CPP
#define PRINT_CPP
#include <string>
#include <iostream>
using namespace std;

void DEBUG_COMMENT(string msg, bool isDebug = true)
{
    if (isDebug)
    {
        cout << "[DEBUG] " << msg << endl;
    }
}
#endif // PRINT_CPP