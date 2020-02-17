
#ifndef __SENDEMAIL_H
#define __SENDEMAIL_H

// uncomment for debug output on Serial port
//#define DEBUG_EMAIL_PORT

// in order to send attachments, this SendEmail class assumes an SD card is mounted as /sdcard

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <FS.h>
#include "SD_MMC.h"

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

  public:
    SendEmail(const String& host, const int port, const String& user, const String& passwd, const int timeout, const bool ssl);

    // attachment is a full path filename to a file on the sd card
    // set attachment to NULL to not include an attachment
    bool send(const String& from, const String& to, const String& subject, const String& msg, const String& attachment);

    ~SendEmail() {client->stop(); delete client;}
};

#endif

