//
// main.c
//
#include <uspienv.h>
#include <uspi.h>
#include <uspios.h>
#include <uspienv/util.h>
#include <uspienv/macros.h>
#include <uspienv/types.h>


static const char FromSample[] = "sample";

int main (void)
{
    if (!USPiEnvInitialize ())
    {
        return EXIT_HALT;
    }
    
    LogWrite (FromSample, LOG_ERROR, "Hello, world");
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

void sig_ign(int code) {
    LogWrite (FromSample, LOG_ERROR, "SIGIGN");
}

void sig_err(int code) {
    LogWrite (FromSample, LOG_ERROR, "SIGERR");

}
