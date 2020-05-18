/**********************************************************************************
 * 
 *  From here to end comment below is the contents of sendemail.h
 *
 *  If an SD card is used, it is assumed to be mounted on /sdcard.
 * 
 */

#ifndef __SENDEMAIL_H
#define __SENDEMAIL_H

// uncomment for debug output on Serial port
//#define DEBUG_EMAIL_PORT

// uncomment if using SD card
#define USING_SD_CARD

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

#ifdef USING_SD_CARD
#include <FS.h>
#include "SD_MMC.h"
#endif


class SendEmail
{
  private:
    const String host;
    const int port;
    const String user;
    const String passwd;
    const int timeout;
    const bool ssl;
    WiFiClient* client;
    String readClient();

    // stuff for attaching buffers (could be images and videos held in memory)
    int attachbuffercount;  // current number of buffer attachments
    static const int attachbuffermaxcount = 10;  // max number of buffers that can be attached
    struct attachbufferitem {
      char * buffername;  // name for buffer
      char * buffer;  // pointer to buffer
      size_t buffersize;  // number of bytes in buffer
    };
    attachbufferitem attachbufferitems[attachbuffermaxcount];
    
#ifdef USING_SD_CARD
    // stuff for attaching files (assumes SD card is mounted as /sdcard)
    int attachfilecount;  // current number of file attachments
    static const int attachfilemaxcount = 10;  // max number of file that can be attached
    struct attachfileitem {
      char * filename;  // name for file
    };
    attachfileitem attachfileitems[attachfilemaxcount];
#endif
    
  public:
    SendEmail(const String& host, const int port, const String& user, const String& passwd, const int timeout, const bool ssl);

    void attachbuffer(char * name, char * bufptr, size_t bufsize);

    void attachfile(char * name);
    
    bool send(const String& from, const String& to, const String& subject, const String& msg);

    void close();
};

#endif

/* end of sendemail.h */
