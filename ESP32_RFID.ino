/*
 * Peringatan : Data akan di simpan pada RFID Card pada sector #1 (blocks #4).
 *
 * tytomulyono.com
 * Typical pin layout used:
 * --------------------------------------
 *             MFRC522      Arduino      
 *             Reader/PCD   Uno/101      
 * Signal      Pin          Pin           
 * --------------------------------------
 * RST/Reset   RST          9             
 * SPI SS      SDA(SS)      10            
 * SPI MOSI    MOSI         11 / ICSP-4   
 * SPI MISO    MISO         12 / ICSP-1   
 * SPI SCK     SCK          13 / ICSP-3   
 * tytomulyono.com
 */

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

bool notif = true;
bool topupSaldo = false;
bool potongSaldo = false;
String input;
String nominal;
long saldo;
int digit;

long saldoLama;
int OLDdigit;

void setup() {
    Serial.begin(115200); // Initialize serial communications with the PC
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("############################################################");

    Serial.println("71190480 - Mini Project IoT - RFID - Sistem Tambah Kurang Saldo Sederhana");

    Serial.println();

    Serial.println("Data akan di simpan pada kartu RFID pada sector #1 (block #4)");

    Serial.println();
}

void loop() {
  if (Serial.available()){
    input = "";
    nominal = "";
    while(Serial.available()>0){
        input = Serial.readStringUntil('\n');
    }
    nominal = getValue(input,' ',1);
    input = getValue(input,' ',0);
    
    if(input.equals("topup")) {
      topupSaldo = true;
      saldo = nominal.toInt();
      saldo = saldo/1000;
      if (saldo > 255){
        saldo = 0;
        Serial.println("Top up tidak boleh lebih dari 255");
      }
      if (saldo < 0){
        saldo = 0;
        Serial.println("Top up tidak boleh kurang dari 0");
      }
      digit = saldo;
      saldo *= 1000;
      Serial.print("Jumlah top up yang di input : ");
      Serial.println(saldo);
      Serial.println("Silahkan tap kartu untuk menambah saldo kartu");
    } else if(input.equals("potong")) {
      potongSaldo = true;
      saldo = nominal.toInt();
      saldo = saldo/1000;
      if (saldo > 255){
        saldo = 0;
        Serial.println("Potongan tidak boleh lebih dari 255");
      }
      if (saldo < 0){
        saldo = 0;
        Serial.println("Jumlah potongan tidak boleh kurang dari 0");
      }
      digit = saldo;
      saldo *= 1000;
      Serial.print("Potongan yang di input : ");
      Serial.println(saldo);
      Serial.println("Silahkan tap kartu untuk memotong saldo kartu");
    } else {
      Serial.println("Input salah!");
    }
  }
  if (notif){
    notif = false;
    Serial.println();
    Serial.println("________________________________________________________________________________");
    Serial.println();
    Serial.println("Silahkan input pada serial monitor dengan format -> aksi (spasi) jumlah (contoh topup 5000 atau potong 2000)");
    Serial.println("Min input: 1000 (dan harus dapat dibagi habis dengan 1000/kelipatan 1000)");
    Serial.println("Max saldo: Rp 255.000,00");
    Serial.println();
    Serial.println("CEK SALDO LANGSUNG TAP");
  }
  if ( ! mfrc522.PICC_IsNewCardPresent()){
      return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()){
      return;
  }
  
  Serial.println();
  Serial.print("Card UID:");
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  
  Serial.println();
  Serial.print("Tipe Kartu : ");
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Cek kesesuaian kartu
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("Kode ini hanya dapat digunakan pada MIFARE Classic cards 1KB - 13.56MHz."));
      notif = true;
      delay(2000);
      resetReader();
      return;
  }

  // that is: sector #1, covering block #4 up to and including block #7
  byte sector         = 1;
  byte blockAddr      = 4;
  
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  // Show the whole sector as it currently is
  //Serial.println("Current data in sector:");
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

/////////////////////////////////////////////////// POTONG SALDO
  if(potongSaldo){
    // Baca Saldo yang ada dari RFID Card
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        resetReader();
        return;
    }
    OLDdigit = buffer[0];
    saldoLama = OLDdigit;
    saldoLama *= 1000;
    Serial.print("Saldo kartu: ");
    Serial.println(saldoLama);
    delay(2000);
    Serial.println("                   ");
    Serial.println("                   ");
  
    // Kurangi Saldo sebesar tagihan merchant
    if (OLDdigit < digit){
      Serial.println("!!! Saldo belum di kurangi, saldo tidak cukup, silahkan isi saldo terlebih dahulu !!!");
      Serial.println("GAGAL Bayar");
      delay(2000);
      Serial.println("                   ");
      Serial.println("                   ");
    
      resetReader();
      return;
    }
  
    OLDdigit -= digit;
    
    byte dataBlock[]    = {
        //0,      1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
        OLDdigit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("GAGAL Write Saldo pada Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
  
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
    }
  
    Serial.println();
  
    Serial.println("Mengurangi Saldo...");
    if (buffer[0] == dataBlock[0]){
      saldo = buffer[0];
      saldo *= 1000;
      //Serial.print("data digit ke 0 : ");
      //Serial.println(buffer[0]);
      Serial.print("======================>>>>>> Saldo kartu sekarang : ");
      Serial.println(saldo);
      Serial.println("_________ Berhasil potong saldo ___________");
      Serial.println("BERHASIL Bayar");
      Serial.print("Sisa Saldo ");
      Serial.println(saldo);
      delay(2000);
     
      Serial.println("                   ");
      Serial.println("                   ");
    }else{
      Serial.println("------------ GAGAL potong saldo --------------");
    }
  }
  
//////////////////////////////////////////////////// TOP UP SALDO
  if (topupSaldo){
    // Baca Saldo yang ada dari RFID Card
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        resetReader();
        return;
    }
    OLDdigit = buffer[0];
    saldoLama = OLDdigit;
    saldoLama *= 1000;
    Serial.print("Saldo Kartu Sebelumnya : ");
    Serial.println(saldoLama);
    Serial.println();
  
    // Tambah saldo dan Write Saldo pada RFID Card
    saldo += saldoLama;
    digit += OLDdigit;
    
    if (digit > 255){
      saldo = 0;
      digit = 0;
      Serial.println("Saldo sebelum di tambah saldo baru melebihi 255 ribu");
      Serial.println("GAGAL top up saldo");
      resetReader();
      return;
    }
    
    byte dataBlock[]    = {
        //0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
        digit, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("GAGAL Write Saldo pada Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
  
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
    }

    Serial.println();
  
    Serial.println("Menambahkan saldo...");
    if (buffer[0] == dataBlock[0]){
      //Serial.print("data digit ke 0 : ");
      //Serial.println(buffer[0]);
      Serial.print("Saldo kartu sekarang : ");
      Serial.println(saldo);
      Serial.println("_________ Berhasil top up saldo pada kartu ___________");
    }else{
      Serial.println("------------ GAGAL TOP UP SALDO --------------");
    }
  }else{
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.println("Gagal Baca Kartu RFID");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        saldo = 0;
        digit = 0;
        resetReader();
        return;
    }

    Serial.println();
  
    Serial.println("Cek Saldo Kartu");
    //Serial.print("data digit ke 0 : ");
    //Serial.println(buffer[0]);
    saldo = buffer[0];
    saldo *= 1000;
    Serial.print("===============>>>> Saldo kartu sekarang : ");
    Serial.println(saldo);
  }

  saldo = 0;
  digit = 0;

  Serial.println();

  // Dump the sector data
  //Serial.println("Current data in sector:");
  //mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  resetReader();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void resetReader(){
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  notif = true;
  topupSaldo = false;
  potongSaldo = false;
}

// Source: https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
