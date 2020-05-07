#include <WiFi.h>
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "FS.h"
#include "SD_MMC.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "sendemail.h"

// only used for this example
#include "KittyFishing.h"

/*
*  to enable/disable email debug on Serial: Uncomment //#define DEBUG_EMAIL_PORT sendemail.h
*/

const char* SSID = "YourSSID"; 
const char* PASS = "YourSSIDPwd"; 

// email server info for the send part, recipient address is set below
const char* emailhost = "smtp.gmail.com";
const int emailport = 465;
const char* emailsendaddr = "YourEmail\@gmail.com";
const char* emailsendpwd = "YourEmailPwd";

// info for the email to be sent  ( the < and > are required for the from and to addresses )
const char* emailfrom = "<FromEmail\@gmail.com>";
const char* emailto = "<ToEmail\@hotmail.com>";

// Setup static IP...
// Set your Static IP address
IPAddress local_IP(192, 168, 2, 32);
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  delay(10);

  Serial.println("");
  
  // Configure static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Mount SD card
  esp_err_t ret = ESP_FAIL;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 10,
  };
  sdmmc_card_t *card;

  Serial.println("Mounting SD card...");
  ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret == ESP_OK) {
    Serial.println("SD card mount successfully!");
  }  else  {
    Serial.printf("Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
  }

  SD_MMC.begin();
  // Done mounting SD card

  // Create an attachment buffer to send with email
  char attach1name[100];
  strcpy( attach1name, "KittyFishing1.jpg" );
  size_t attach1bufferlen = sizeof(KittyFishing);
  uint8_t * attach1buffer = (uint8_t*) malloc( attach1bufferlen );
  memcpy( attach1buffer, (const uint8_t *) KittyFishing, attach1bufferlen );

  // Create another attachment buffer to send with email
  char attach2name[100];
  strcpy( attach2name, "KittyFishing2.jpg" );
  size_t attach2bufferlen = sizeof(KittyFishing);
  uint8_t * attach2buffer = (uint8_t*) malloc( attach2bufferlen );
  memcpy( attach2buffer, (const uint8_t *) KittyFishing, attach2bufferlen );


  // Create an attachment file to send with email
  File kitty;

  char attach3name[100];
  strcpy( attach3name, "/KittyFishing3.jpg" );
  kitty = SD_MMC.open(attach3name,"w");
  kitty.write( (const uint8_t *) KittyFishing, sizeof(KittyFishing) );
  kitty.close();

  delay(3);

  // Create another attachment file to send with email
  char attach4name[100];
  strcpy( attach4name, "/KittyFishing4.jpg" );
  kitty = SD_MMC.open(attach4name,"w");
  kitty.write( (const uint8_t *) KittyFishing, sizeof(KittyFishing) );
  kitty.close();

  delay(3);

  // sendemail stuff
  String msg = "This is a test email.\r\nHopefully the attachments made it too.";
  String subject = "Test Email";

  Serial.print("\nHopefully it will email:\n");
  Serial.println(msg);

  // create SendEmail object 
  SendEmail e( emailhost, emailport, emailsendaddr, emailsendpwd, 5000, true); 

  // add attachments to email
  Serial.println("\nAdding email attachment 1...");
  e.attachbuffer( attach1name, (char *) attach1buffer, attach1bufferlen );

  Serial.println("\nAdding email attachment 2...");
  e.attachbuffer( attach2name, (char *) attach2buffer, attach2bufferlen );

  Serial.println("\nAdding email attachment 3...");
  e.attachfile( attach3name );

  Serial.println("\nAdding email attachment 4...");
  e.attachfile( attach4name );

  // Send Email
  Serial.println("\nSending email...");

  bool result = e.send( emailfrom, emailto, subject, msg );

  if ( result ) { 
    Serial.println("\nEmail successfully sent.");
  } else {
    Serial.println("\nSending email failed.");
  }

  // close email connection
  e.close();

  free( attach1buffer );
  free( attach2buffer );
}

void loop() { 

}
