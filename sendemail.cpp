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
  attachbufferitems[attachbuffercount].buffername = (char *) malloc( strlen(name)+1 );
  strcpy( attachbufferitems[attachbuffercount].buffername, name ); 
  attachbufferitems[attachbuffercount].buffer = bufptr;
  attachbufferitems[attachbuffercount].buffersize = bufsize;
  
  attachbuffercount++;
}

#ifdef USING_SD_CARD
void SendEmail::attachfile(char * name)
{
  attachfileitems[attachfilecount].filename = (char *) malloc( strlen(name)+8 );
  strcpy( attachfileitems[attachfilecount].filename, "/sdcard" );
  strcat( attachfileitems[attachfilecount].filename, name );
  
  attachfilecount++;
}
#endif


bool SendEmail::send(const String& from, const String& to, const String& subject, const String& msg)
{
  if (!host.length())
  {
    return false;
  }
  String buffer2((char *)0);  // create String without memory allocation
  buffer2.reserve(800);  // now allocate bytes for string, really should only use 780 of it
  client->stop();
  client->setTimeout(timeout);
  // smtp connect
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("Connecting: "));
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
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
    buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
  Serial.println(buffer);
#endif
  if (!buffer.startsWith(F("250")))
  {
    return false;
  }
  buffer = F("DATA");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = readClient();
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("SERVER->CLIENT: "));
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
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = F("To: ");
  buffer += to;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = F("Subject: ");
  buffer += subject;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  // setup to send message body
  buffer = F("MIME-Version: 1.0\r\n");
  buffer += F("Content-Type: multipart/mixed; boundary=\"{BOUNDARY}\"\r\n\r\n");
  buffer += F("--{BOUNDARY}\r\n");
  buffer += F("Content-Type: text/plain\r\n");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = msg;
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  // process buffer attachments
  for ( int i = 0; i < attachbuffercount; i++ ) {
    char * pos = attachbufferitems[i].buffer;
    size_t alen = attachbufferitems[i].buffersize;
    base64 b;
    
    buffer = F("\r\n--{BOUNDARY}\r\n");
    buffer += F("Content-Type: application/octet-stream\r\n");
    buffer += F("Content-Transfer-Encoding: base64\r\n");
    buffer += F("Content-Disposition: attachment;filename=\"");
    buffer += attachbufferitems[i].buffername;
    buffer += F("\"\r\n");
    client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
  size_t bytessent = 0;
#endif
    // read data from buffer, base64 encode and send it
    // 3 binary bytes (57) becomes 4 base64 bytes (76)
    // plus CRLF is ideal for one line of MIME data
    // 570 bytes will be read at a time and sent as ten lines of base64 data
    size_t flen = 570;  
    uint8_t * fdata = (uint8_t*) malloc( flen );
    if ( alen < flen ) flen = alen;
    // read data from buffer
    memcpy( fdata, pos, flen ); 
    delay(10);
    buffer2 = "";
    size_t bytecount = 0;
    while ( flen > 0 ) {
      while ( flen > 56 ) {
        // convert to base64 in 57 byte chunks
        buffer2 += b.encode( fdata+bytecount, 57 );
        // tack on CRLF
        buffer2 += "\r\n";
        bytecount += 57;
        flen -= 57;
      }
      if ( flen > 0 ) {
        // convert last set of bytes to base64
        buffer2 += b.encode( fdata+bytecount, flen );
        // tack on CRLF
        buffer2 += "\r\n";
      }
      // send the lines in buffer
      client->println( buffer2 );
#ifdef DEBUG_EMAIL_PORT
  bytessent += bytecount + flen;
  Serial.print(F(" bytes sent so far: ")); Serial.println(bytessent);
#endif
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
    fdata = NULL;
  }

#ifdef USING_SD_CARD
  // process file attachments
  for ( int i = 0; i < attachfilecount; i++ ) {
    FILE *atfile = NULL;
    char aname[110];
    char * pos = NULL;  // points to actual file name
    size_t flen;
    base64 b;
    if ( atfile = fopen(attachfileitems[i].filename, "r") ) {
      // get filename from attachment name
      pos = strrchr( attachfileitems[i].filename, '/' ) + 1;
      // attachment will be pulled from the file named
      buffer = F("\r\n--{BOUNDARY}\r\n");
      buffer += F("Content-Type: application/octet-stream\r\n");
      buffer += F("Content-Transfer-Encoding: base64\r\n");
      buffer += F("Content-Disposition: attachment;filename=\"");
      buffer += pos ;
      buffer += F("\"\r\n");
      client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
  size_t bytessent = 0;
#endif
      // read data from file, base64 encode and send it
      // 3 binary bytes (57) becomes 4 base64 bytes (76)
      // plus CRLF is ideal for one line of MIME data
      // 570 bytes will be read at a time and sent as ten lines of base64 data
      uint8_t * fdata = (uint8_t*) malloc( 570 );
      // read data from file
      flen = fread( fdata, 1, 570, atfile );
      delay(10);
      buffer2 = "";
      int lc = 0;
      size_t bytecount = 0;
      while ( flen > 0 ) {
        while ( flen > 56 ) {
          // convert to base64 in 57 byte chunks
          buffer2 += b.encode( fdata+bytecount, 57 );
          // tack on CRLF
          buffer2 += "\r\n";
          bytecount += 57;
          flen -= 57;
        }
        if ( flen > 0 ) {
          // convert last set of bytes to base64
          buffer2 += b.encode( fdata+bytecount, flen );
          // tack on CRLF
          buffer2 += "\r\n";
        }
        // send the lines in buffer
        client->println( buffer2 );
#ifdef DEBUG_EMAIL_PORT
  bytessent += bytecount + flen;
  Serial.print(F(" bytes sent so far: ")); Serial.println(bytessent);
#endif
        buffer2 = "";
        bytecount = 0;
        delay(10);
        flen = fread( fdata, 1, 570, atfile );
      }
      fclose( atfile );
      free( fdata );
      fdata = NULL;
    } 
  }
#endif

  buffer = F("\r\n--{BOUNDARY}--\r\n.");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  buffer = F("QUIT");
  client->println(buffer);
#ifdef DEBUG_EMAIL_PORT
  Serial.print(F("CLIENT->SERVER: "));
  Serial.println(buffer);
#endif
  return true;
}

void SendEmail::close() {

  // cleanup buffer attachments
  for ( int i = 0; i < attachbuffercount; i++ ) {
    free( attachbufferitems[i].buffername );
    attachbufferitems[i].buffername = NULL;
  }

#ifdef USING_SD_CARD
  // cleanup file attachments
  for ( int i = 0; i < attachfilecount; i++ ) {
    free( attachfileitems[i].filename );
    attachfileitems[i].filename = NULL;
  }
#endif
  
  client->stop();
  delete client;
}
