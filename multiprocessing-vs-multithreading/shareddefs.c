#include "shareddefs.h"
#include <string.h>

void getFileName(char name[], int id)
{
    strcpy(name, MQNAME);
    int nameLength = strlen(name);
    name[nameLength] = '0' + id;
    name[nameLength + 1] = '\0';
}