//
// main.c
//

#include <stddef.h>
#include <stdlib.h>
#include "Python.h"



static wchar_t *argv[2] = { L"python", L"--help" };

void initmsg(void);
int USPiEnvInitialize (void);
void USPiEnvClose (void);

#define EXIT_HALT	0

int main (void)
{
    if (!USPiEnvInitialize ())
    {
        return EXIT_HALT;
    }
    
    initmsg();
    
    setenv("PYTHONHASHSEED", "0", 1); // No random numbers available
    
    char *t;
    t = getenv("PYTHONHASHSEED");
    write(0, "HASHSEED=", 9);

    write(0, t, strlen(t));
    
    Py_Main(2, argv);
/*    
    if (!USPiInitialize ())
    {
        LogWrite (FromSample, LOG_ERROR, "Cannot initialize USPi");

        USPiEnvClose ();

        return EXIT_HALT;
    }

*/

    
    
    
    USPiEnvClose ();

    return EXIT_HALT;
}


