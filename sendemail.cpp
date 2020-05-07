/*****************************************************************************
 * 
 *  From here to the end is the contents of sendemail.cpp
 *  This version can send text or binary objects (jpg, avi) from memory as attachments
 *  and can send files from the SD card mounted on /sdcard.
 *  
 */

#include "sendemail.h"

SendEmail::SendEmail(const String& host, const int port, const String& user, const String& passwd, const int timeout, const bool ssl) :
    host(host), port(port), user(user), passwd(passwd), timeout(timeout), ssl(ssl), client((ssl) ? new WiFiClientSecure() : new WiFiClient())
{
  attachbuffercount = 0;
#ifdef USING_SD_CARD
  attachfilecount = 0;
#endif
}

String SendEmail::readClient()
{
  String r = client->readStringUntil('\n');
  r.trim();
  while (client->available()) r += client->readString();
  return r;
}

void SendEmail::attachbuffer(char * name, char * bufptr, size_t bufsize)
{
  strcpy( attachbufferitems[attachbuffercount].buffername, name );
  attachbufferitems[attachbuffercount].buffer = bufptr;
  attachbufferitems[attachbuffercount].buffersize = bufsize;
  
  attachbuffercount++;
}

#ifdef USING_SD_CARD
void SendEmail::attachfile(char * name)
{
  strcpy( attachfileitems[attachfilecount].filename, name );
  
  attachfilecount++;
}
#endif


bool SendEmail::send(const String& from, const String& to, const String& subject, const String& msg)
{
  if (!host.length())
  {
    return false;
  }
  client->stop();
  client->setTimeout(timeout);
  // smtp connect
#ifdef DEBUG_EMAIL_PORT
  Serial.print("Connecting: ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
#endif
  if (!client->connect(host.c_str(), port))
  {
    return false;
  }
  String buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  // note: F(..) as used below puts the string in flash instead of RAM
  if (!buffer.startsWith(F("220")))
  {
    return false;
  }
  buffer = F("HELO ");
  buffer += client->localIP();
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  if (user.length()>0  && passwd.length()>0 )
  {
    buffer = F("AUTH LOGIN");
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
    if (!buffer.startsWith(F("334")))
    {
      return false;
    }
    base64 b;
    buffer = user;
    buffer = b.encode(buffer);
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
    if (!buffer.startsWith(F("334")))
    {
      return false;
    }
    buffer = this->passwd;
    buffer = b.encode(buffer);
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
    if (!buffer.startsWith(F("235")))
    {
      return false;
    }
  }
  // smtp send mail
  buffer = F("MAIL FROM: ");
  buffer += from;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  buffer = F("RCPT TO: ");
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  buffer = F("DATA");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print("SERVER->CLIENT: ");
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("354")))
  {
    return false;
  }
  buffer = F("From: ");
  buffer += from;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = F("To: ");
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = F("Subject: ");
  buffer += subject;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  // setup to send message body
  buffer = F("MIME-Version: 1.0\r\n");
  buffer += F("Content-Type: multipart/mixed; boundary=\"{BOUNDARY}\"\r\n\r\n");
  buffer += F("--{BOUNDARY}\r\n");
  buffer += F("Content-Type: text/plain\r\n");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = msg;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  // process buffer attachments
  for ( int i = 0; i < attachbuffercount; i++ ) {
    char aname[100];
    strcpy( aname, attachbufferitems[i].buffername );
    char * pos = attachbufferitems[i].buffer;
    size_t alen = attachbufferitems[i].buffersize;
    base64 b;
    
    buffer = F("\r\n--{BOUNDARY}\r\n");
    buffer += F("Content-Type: application/octet-stream\r\n");
    buffer += F("Content-Transfer-Encoding: base64\r\n");
    buffer += F("Content-Disposition: attachment;filename=\"");
    buffer += aname ;
    buffer += F("\"\r\n");
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
    // read data from buffer, base64 encode and send it
    // 3 binary bytes (57) becomes 4 base64 bytes (76)
    // plus CRLF is ideal for one line of MIME data
    // 570 byes will be read at a time and sent as ten lines of base64 data
    size_t flen = 570;  
    uint8_t * fdata = (uint8_t*) malloc( flen );
    if ( alen < flen ) flen = alen;
    // read data from buffer
    memcpy( fdata, pos, flen ); 
    String buffer2 = "";
    size_t bytecount = 0;
    while ( flen > 0 ) {
      while ( flen > 56 ) {
        // convert to base64 in 57 byte chunks
        buffer = b.encode( fdata+bytecount, 57 );
        buffer2 += buffer;
        // tack on CRLF
        buffer2 += "\r\n";
        bytecount += 57;
        flen -= 57;
      }
      if ( flen > 0 ) {
        // convert last set of byes to base64
        buffer = b.encode( fdata+bytecount, flen );
        buffer2 += buffer;
        // tack on CRLF
        buffer2 += "\r\n";
      }
      // send the lines in buffer
      client->println( buffer2 );
      buffer2 = "";
      delay(10);
      // calculate bytes left to send
      alen = alen - bytecount - flen;
      pos = pos + bytecount + flen;
      flen = 570;
      if ( alen < flen ) flen = alen;
      // read data from buffer
      if ( flen > 0 ) memcpy( fdata, pos, flen ); 
      bytecount = 0;
    }
    free( fdata );
  }

#ifdef USING_SD_CARD
  // process file attachments
  for ( int i = 0; i < attachfilecount; i++ ) {
    FILE *atfile = NULL;
    char aname[110];
    char * pos = NULL;
    size_t flen;
    base64 b;
    // full path to file on SD card
    strcpy( aname, "/sdcard" );
    strcat( aname, attachfileitems[i].filename );
    if ( atfile = fopen(aname, "r") ) {
      // get filename from attachment name
      pos = strrchr( aname, '/' );
      strcpy( aname, pos+1 );
      // attachment will be pulled from the file named
      buffer = F("\r\n--{BOUNDARY}\r\n");
      buffer += F("Content-Type: application/octet-stream\r\n");
      buffer += F("Content-Transfer-Encoding: base64\r\n");
      buffer += F("Content-Disposition: attachment;filename=\"");
      buffer += aname ;
      buffer += F("\"\r\n");
      client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
      // read data from file, base64 encode and send it
      // 3 binary bytes (57) becomes 4 base64 bytes (76)
      // plus CRLF is ideal for one line of MIME data
      // 570 byes will be read at a time and sent as ten lines of base64 data
      uint8_t * fdata = NULL;  
      fdata = (uint8_t*) malloc( 570 );
      // read data from file
      flen = fread( fdata, 1, 570, atfile );
      String buffer2 = "";
      int lc = 0;
      size_t bytecount = 0;
      while ( flen > 0 ) {
        while ( flen > 56 ) {
          // convert to base64 in 57 byte chunks
          buffer = b.encode( fdata+bytecount, 57 );
          buffer2 += buffer;
          // tack on CRLF
          buffer2 += "\r\n";
          bytecount += 57;
          flen -= 57;
        }
        if ( flen > 0 ) {
          // convert last set of byes to base64
          buffer = b.encode( fdata+bytecount, flen );
          buffer2 += buffer;
          // tack on CRLF
          buffer2 += "\r\n";
        }
        // send the lines in buffer
        client->println( buffer2 );
        buffer2 = "";
        bytecount = 0;
        delay(10);
        flen = fread( fdata, 1, 570, atfile );
      }
      fclose( atfile );
      free( fdata );
    } 
  }
#endif

  buffer = F("\r\n--{BOUNDARY}--\r\n.");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  buffer = F("QUIT");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print("CLIENT->SERVER: ");
  Serial.println(buffer);
#endif
  return true;
}
