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

const char* SSID = "WilliamsHome4"; 
const char* PASS = "3103864271"; 

// email server info for the send part, recipient address is set below
const char* emailhost = "smtp.gmail.com";
const int emailport = 465;
const char* emailsendaddr = "88mtnbkr\@gmail.com";
const char* emailsendpwd = "W3lcome4";

// info for the email to be sent  ( the < and > are required for the from and to addresses )
const char* emailfrom = "<88mtnbkr\@gmail.com>";
const char* emailto = "<ewilliams41\@hotmail.com>";

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

  // Create an attachment file to send with email
  File kitty = SD_MMC.open("/KittyFishing.jpg","w");
  kitty.write( (const uint8_t *) KittyFishing, sizeof(KittyFishing) );
  kitty.close();

  delay(3);

  // sendemail stuff
  String msg = "This is a test email.\r\nHopefully the attachment made it too.";
  String subject = "Test Email";
  String attachment = "/KittyFishing.jpg";

  Serial.print("\nHopefully it will email:\n");
  Serial.println(msg);

  // create SendEmail object 
  SendEmail e( emailhost, emailport, emailsendaddr, emailsendpwd, 5000, true); 

  // Send Email
  Serial.println("\nSendEmail object created, now trying to send...");

  e.send( emailfrom, emailto, subject, msg, attachment );
 
  Serial.println("\nEmail was hopefully sent.");

}

void loop() { 

}
