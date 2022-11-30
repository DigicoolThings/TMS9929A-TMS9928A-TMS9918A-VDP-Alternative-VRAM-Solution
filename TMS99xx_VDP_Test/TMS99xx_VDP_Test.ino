/* TMS99xx (TMS9918 / TMS9928 / TMS9929) VDP (and VRAM) Test
*  (should also work for later TMS9118 / TMS9128 / TMS9129 VDP).
*  
*  Using an Arduino Uno (but any board should do just fine).
*  
*  Connections:
*  TMS       MODE CSW  CSR  RESET  CD7 CD6 CD5 CD4 CD3 CD2 CD1 CD0
*  Arduino   D2   D3   D8   D9     A0  A1  A2  A3  D4  D5  D6  D7 
*  (port)    PD2  PD3  PB0  PB1    PC0 PC1 PC2 PC3 PD4 PD5 PD6 PD7
*
* VDP IC is tested via stepping through the 14 different Backdrop (and Border) colors.
* VRAM is utilised for displaying a single 8x8 Graphics 1 pattern in all 768 screen positions.
* testVRAM() does a multi-pass test of the VRAM using either a nibble or byte wide pattern,  
*   allowing for testing with only nibble wide VRAM (eg. I was testing a circuit with a single 4464).  
*
* Open your Serial Monitor for progress and VRAM test result reporting. :)
*
* Refer inline comments for more detailed functional description.
*
*  Created: Mar 2022
*  by: Greg@DigicoolThings.com
*/

/*
* TMS Control Pins
*/
#define MODE  2
#define CSW   3
#define CSR   8
#define RESET 9

/*
* MODE pin options
*/
#define VRAM_ACCESS     LOW
#define REGISTER_ACCESS HIGH

/*-----------------------------------------------------------------------------
* Core VDP read & write access functions
* These can be called directly, or use the Convenience functions following
=============================================================================*/

uint8_t readByteVDP(bool vramOrRegister) {
/*
* Reads a byte from VDP
* vramOrRegister specifies access MODE 
* VRAM access = LOW, Register access = HIGH 
* nb. Can be called directly or see Convenience functions below
*/
  uint8_t dataByte = 0;
  // Set data IO pins as inputs
  DDRD = DDRD & 0x0F; // PD4..PD7 as inputs
  DDRC = DDRC & 0xF0; // PC0..PC3 as inputs
  // Set MODE pin for Register or VRAM access
  digitalWrite(MODE, vramOrRegister);        
  // Pulse Chip Select Read and read byte 
  digitalWrite(CSR, LOW);
  dataByte = (PIND & 0xF0) | (PINC & 0x0F);            
  digitalWrite(CSR, HIGH);
  return dataByte;
}

void writeByteVDP(uint8_t dataByte, bool vramOrRegister) {
/*
* Writes a byte to the VDP
* vramOrRegister specifies accesss MODE 
* VRAM access = LOW, Register access = HIGH 
* nb. Can be called directly or see Convenience functions below
*/
  // Set data IO pins as outputs
  DDRD = DDRD | 0xF0; // PD4..PD7 as outputs
  DDRC = DDRC | 0x0F; // PC0..PC3 as outputs
  // Set output data byte  
  PORTD = (PORTD & 0x0F) | (dataByte & 0xF0);
  PORTC = (PORTC & 0xF0) | (dataByte & 0x0F);    
  // Set MODE pin for Register or VRAM access
  digitalWrite(MODE, vramOrRegister);        
  // Pulse Chip Select Write 
  digitalWrite(CSW, LOW);            
  digitalWrite(CSW, HIGH);
}

/*-----------------------------------------------------------------------------
* Additional fundamental VDP access functions
* VRAM access address setup, and VDP Register writes
=============================================================================*/

void setVramReadAddress(uint16_t vramAddress) {
  writeByteVDP(vramAddress & 0x00FF, REGISTER_ACCESS);
  writeByteVDP( (vramAddress >> 8) & 0x003F, REGISTER_ACCESS);
}

void setVramWriteAddress(uint16_t vramAddress) {
  writeByteVDP(vramAddress & 0x00FF, REGISTER_ACCESS);
  writeByteVDP( ((vramAddress >> 8) & 0x003F) | 0x40, REGISTER_ACCESS);
}

void writeRegisterByteVDP(uint8_t dataByte, uint8_t registerNum) {
/*
* Writes a byte to a VDP register
* registerNum specifies which register (0..7) 
*/
  writeByteVDP(dataByte, REGISTER_ACCESS);
  // Set Register number for VDP to put the data into
  writeByteVDP( (registerNum & 0x07) | 0x80, REGISTER_ACCESS);
}

/*-----------------------------------------------------------------------------
* VDP access Convenience functions follow
* VRAM data read & write, and VDP Status register read
=============================================================================*/

uint8_t readStatusByteVDP() {
/*
* Reads VDP Status Register byte
* nb. This is a Convenience function only
*/
  return readByteVDP(REGISTER_ACCESS);
}

uint8_t readVramByteVDP() {
/*
* Reads VDP VRAM byte
* First setVramReadAddress() prior to 1 or more readVramByteVDP() calls  
* nb. This is a Convenience function only
*/
  return readByteVDP(VRAM_ACCESS);
}

void writeVramByteVDP(uint8_t dataByte) {
/*
* Writes a byte to a VDP register
* First setVramWriteAddress() prior to 1 or more writeVramByteVDP() calls  
* nb. This is a Convenience function only
*/
  writeByteVDP(dataByte, VRAM_ACCESS);
}

/*-----------------------------------------------------------------------------
* General Utility functions follow
=============================================================================*/

static int num_hex_digits(unsigned n)
{
    if (!n) return 1;

    int ret = 0;
    for (; n; n >>= 4) {
        ++ret;
    }
    return ret;
}

void hex_string(unsigned n, int len, char *s) {
/*
* len specifies the number of hex digits required,
* len = 0 specifies to self determine required even length
* char specifies a char array of at least len+1 length
*/
  const char hex_lookup[] = "0123456789ABCDEF";
  if (!len) {
    len = num_hex_digits(n);

    if (len & 1) {
      *s++ = '0';
    }
  }
    
  s[len] = '\0';

  for (--len; len >= 0; n >>= 4, --len) {
      s[len] = hex_lookup[n & 0x0F];
  }
}

void bit_string(unsigned n, int len, char *s) {
/*
* len specifies the number of bit digits required
* len = 0 specifies a default byte (8) bit string length
* char specifies a char array of at least len+1 length
*/
  if (!len) len = 8;

  s[len] = '\0';

  for (--len; len >= 0; n >>= 1, --len) {
      s[len] = (n & 0x01) ? '1' : '0';
  }
}

static int vramTest() {
/*
* Multi-pass test of the VRAM using either a nibble or byte pattern
* Select a testValues[] array with either byte or nibble only values
* ie. If testing with a single 4464 DRAM chip located in the lower nibble.
*/
// Either, Byte wide memory testing values
  uint8_t testValues[] = {0x00, 0xFF, 0x55, 0xAA};
// Or, Low nibble only testing values (high nibble zeroed)
//  uint8_t testValues[] = {0x00, 0x0F, 0x05, 0x0A};

  char hexString[9];
  bool testFail = false;
  uint8_t testByte;
  uint16_t testAddr;
  uint8_t byteRead;
   
  Serial.println( "Testing 16KB VRAM...");
  for(uint8_t testValueIndex = 0; testValueIndex < sizeof(testValues); testValueIndex++) {
    testByte = testValues[testValueIndex];
    bit_string(testByte, 8, hexString);
    
    Serial.println( "Testing Pattern: 0b" + String(hexString) );
    setVramWriteAddress(0x0000);
    for (uint16_t lp = 0; lp<16384; lp++) {
      writeVramByteVDP(testByte);
    }

    setVramReadAddress(0x0000);
    for (uint16_t lp = 0; lp<16384; lp++) {
      testAddr = lp;
      
      byteRead = readVramByteVDP();
// In lower nibble only testing you might want to mask for lower nibble only
//      byteRead = readVramByteVDP()  & 0x0F;

      if (byteRead != testByte) {
        testFail = true;
        break;
      }
    }
  
    if (testFail) {break;} 
  }

  if (testFail) {
    Serial.println( "Test Failure! :(");
    hex_string(testAddr, 4, hexString);
    Serial.print( "Address: 0x" + String(hexString));
    bit_string(testByte, 8, hexString);
    Serial.print( " > Wrote: 0b" + String(hexString));
    bit_string(byteRead, 8, hexString);
    Serial.println( " Read: 0b" + String(hexString));
    return 0;
  } else {
    Serial.println( "All Tests Passed! :)" );
    return 1;
  }
}

void vramClear() {
/*
* Clear 16KB VRAM by writing 0x00 to all locations
*/
  Serial.println("Clear 16KB VRAM");

  setVramWriteAddress(0x0000);
  for (uint16_t lp = 0; lp<16384; lp++) {
    writeVramByteVDP(0x00);
  }
}

void vramSetup() {
/*
* VRAM Pattern, Name, and Color Tables setup 
*  for Graphics I Mode tests 
*/
  char hexString[9];
  
  // Setup Test Pattern 0 in Pattern Table
  Serial.println("Setup Test Pattern 0 in Pattern Table");
  setVramWriteAddress(0x0800);
  // Use following for loop if you'd like to repeat the pattern
//  for (uint16_t lp = 0; lp<2048; lp=lp+8) {

  // Pattern using low nibble only (eg. for single 4464 only testing)  
/*   
    writeVramByteVDP(0x00); // 00000000
    writeVramByteVDP(0x04); // 00000100
    writeVramByteVDP(0x0A); // 00001010
    writeVramByteVDP(0x04); // 00000100
    writeVramByteVDP(0x0E); // 00001110
    writeVramByteVDP(0x04); // 00000100
    writeVramByteVDP(0x0A); // 00001010
    writeVramByteVDP(0x00); // 00000000
*/

  // Pattern using full byte pattern (for dual 4464 or other bytewide memory testing)
    writeVramByteVDP(0x00); // 00000000
    writeVramByteVDP(0x18); // 00011000
    writeVramByteVDP(0x24); // 00100100
    writeVramByteVDP(0x42); // 01000010
    writeVramByteVDP(0x7E); // 01111110
    writeVramByteVDP(0x42); // 01000010
    writeVramByteVDP(0x42); // 01000010
    writeVramByteVDP(0x00); // 00000000 

//   }

  // Optional simple VRAM Test by reading and reporting the Pattern 0 just stored in Pattern Table
/*
  Serial.println("Reading back Test Pattern 0 from Pattern Table");
  setVramReadAddress(0x0800);
  for (uint16_t lp = 0; lp<8; lp++) {
    hex_string(readVramByteVDP(), 2, hexString);
    Serial.println("Read Byte value: 0x" + String(hexString));
  }
*/

  // Set Pattern 0 to be in all 768 Screen locations
  Serial.println("Setup Name Table");
  setVramWriteAddress(0x1400);
  for (uint16_t lp = 0; lp<768; lp++) {
    writeVramByteVDP(0x00);
  }

  // Set Color (for Patterns 0 - 7)
/*  
* Upper nibble defines color of "on" bits (1's)   
* Lower nibble defines color of "off" bits (0's)
* nb. If we are testing VRAM with a single lower nibble 4464 then Pattern "on" bits will all be transparent (color 0)
*/
  Serial.println("Setup Color Table");
  setVramWriteAddress(0x2000);
  // The following pattern color is the only one that will work with low nibble only memory (Transparent on bits)
//  writeVramByteVDP(0x01); // Transparent (color 0) on bits, Black (color 1) off bits 

  // The following pattern colors require byte wide memory (on bits a color other than Transparent)
//  writeVramByteVDP(0x11); // Black (color 1) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x21); // Medium Green (color 2) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x31); // Light Green (color 3) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x41); // Dark Blue (color 4) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x51); // Light Blue (color 5) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x61); // Dark Red (color 6) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x71); // Cyan (color 7) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x81); // Medium Red (color 8) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0x91); // Light Red (color 9) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0xA1); // Dark Yellow (color 10) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0xB1); // Light Yellow (color 11) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0xC1); // Dark Green (color 12) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0xD1); // Magenta (color 13) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0xE1); // Grey (color 14) on bits, Black (color 1) off bits 
//  writeVramByteVDP(0xF1); // White (color 15) on bits, Black (color 1) off bits 

  // This is my favorite combination: White for on bits and Dark Blue for off bits
  writeVramByteVDP(0xF4); // White (color 15) on bits, Dark Blue (color 4) off bits 
  
  // Following example will instead setup the same Color for all 256 possible Patterns
/*  
  for (uint16_t lp = 0; lp<32; lp++) {
    writeVramByteVDP(0x01);
  }
*/
}

/*-----------------------------------------------------------------------------
* Arduino standard Setup() & loop() functions follow  
=============================================================================*/

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  // Initialise TMS Control Pins as outputs and not asserted
  pinMode(MODE, OUTPUT);
  pinMode(CSW, OUTPUT);
  digitalWrite(CSW, HIGH);    
  pinMode(CSR, OUTPUT);
  digitalWrite(CSR, HIGH);  
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);

  // Reset TMS VDP
  delayMicroseconds(5);
  digitalWrite(RESET, LOW);
  delayMicroseconds(5);  
  digitalWrite(RESET, HIGH);

  // Initialise VDP Registers
/*
* Register 0
* bit 0 - External VDP Plane Disable (0) / Enable (1)
* bit 1 - Display Mode M3 - Graphics I, Multicolor, or Text Mode (0) / Graphics II Mode (1)
* bits 2..7 unused (0)
*/
  writeRegisterByteVDP(0x00, 0); // Graphics I Mode
//  writeRegisterByteVDP(0x02, 0); // Graphics II Mode

/*
* Register 1
* bit 0 - Sprite Magnification Off (0) / x2 (1)
* bit 1 - Sprite Size 8x8 pixels (0) / 16x16 pixels (1)
* bit 2 - unused (0)
* bit 3 - Display Mode M2 - Graphics I, Graphics II, or Text Mode (0) / Multicolor Mode (1)
* bit 4 - Display Mode M1 - Graphics I, Graphics II, or Multicolor Mode (0) / Text Mode (1)
* bit 5 - Interrupt Disabled (0) / Enabled (1)
* bit 6 - Display Area Disabled (0) / Enabled (1)
* bit 7 - VRAM Size 4KB (0) / 16KB (1)
*/
  writeRegisterByteVDP(0xC0, 1); // Graphics I or II Mode, 8x8 Sprites, 16KB VRAM, Display Area Enabled
//  writeRegisterByteVDP(0xE0, 1); // as above, Interrupts Enabled  
//  writeRegisterByteVDP(0xC8, 1); // Multicolor Mode, 8x8 Sprites, 16KB VRAM, Display Area Enabled
//  writeRegisterByteVDP(0xE8, 1); // as above, Interrupts Enabled  
//  writeRegisterByteVDP(0xD0, 1); // Text Mode, 8x8 Sprites, 16KB VRAM, Display Area Enabled
//  writeRegisterByteVDP(0xF0, 1); // as above, Interrupts Enabled  

/*
* Register 2
* bits 0..3 - Name table start address (* 0x0400) 0x00 - 0x0F
* bits 4..7 - unused (0)
*/
  writeRegisterByteVDP(0x05, 2); // Name table start address = 0x1400

/*
* Register 3
* bits 0..7 - Color table start address (* 0x0040) 0x00 - 0xFF
*/
  writeRegisterByteVDP(0x80, 3); // Color table start address = 0x2000
  
/*
* Register 4
* bits 0..2 - Pattern table start address (* 0x0800) 0x00 - 0x07
* bits 3..7 - unused (0)
* nb. Refer manual for 0x0000 or 0x2000 address restriction in Graphics II Mode
*/
  writeRegisterByteVDP(0x01, 4); // Pattern table start address = 0x0800

/*
* Register 5
* bits 0..6 - Sprite Attribute table start address (* 0x0080) 0x00 - 0x7F
* bit 7 - unused (0)
* nb. Refer manual for 0x0000 or 0x2000 address restriction in Graphics II Mode
*/
  writeRegisterByteVDP(0x20, 5); // Sprite Attribute table start address = 0x1000

/*
* Register 6
* bits 0..2 - Sprite Pattern table start address (* 0x0800) 0x00 - 0x07
* bits 3..7 - unused (0)
*/
  writeRegisterByteVDP(0x00, 6); // Sprite Pattern table start address = 0x0000

/*
* Register 7
* bits 0..3 - Text Mode off bit and Border / Backdrop color 0x00 - 0x0F
* bits 4..7 - Text Mode on bit color 0x00 - 0x0F
*/
  writeRegisterByteVDP(0x01, 7); // Transparent Text / Black Backdrop

  // Use the following VRAM call code block if you only want single initial VRAM test (and setup)
/**/  
  // Perform 16KB VRAM test
  vramTest();
 
  // Clear 16KB VRAM
  vramClear();

  // Setup VRAM Pattern, Name, and Color Tables for Graphics I Mode tests 
  vramSetup();
/**/

}

uint8_t backdropColor = 1;
char *VDPColor[] = {"Transparent", "Black", "Medium Green", "Light Green",
                    "Dark Blue", "Light Blue", "Dark Red", "Cyan",
                    "Medium Red", "Light Red", "Dark Yellow", "Light Yellow",
                    "Dark Green", "Magenta", "Gray", "White"             
                   };
uint16_t testCountSuccess = 0;
uint16_t testCountFail = 0;

void loop() {
  // Use the following vram call etc code block if you want repeated VRAM tests (and subsequent VRAM setup)
/*  
  Serial.println();
  
  // Optional - Disable the Display Area during VRAM Test
  writeRegisterByteVDP(0x80, 1); // Graphics I or II Mode, 8x8 Sprites, 16KB VRAM, Display Area Disabled

  // Perform 16KB VRAM test
  (vramTest()) ? testCountSuccess++ : testCountFail++;
  Serial.println("VRAM Test counts: Success = "+String(testCountSuccess)+" / Fail = " + String(testCountFail)+"\n");
 
  // Clear 16KB VRAM
  vramClear();

  // Setup VRAM Pattern, Name, and Color Tables for Graphics I Mode tests 
  vramSetup();

  // Reenable the Display Area after VRAM Test (only necessary if we disabled it above)
  writeRegisterByteVDP(0xC0, 1); // Graphics I or II Mode, 8x8 Sprites, 16KB VRAM, Display Area Enabled
*/  

  
  // Rotate through the 14 different Backdrop colors (2..15)
  // Excludes Transparent (0) and Black (1)
  backdropColor++;
  if (backdropColor > 15) backdropColor = 2;

  Serial.println("Testing Backdrop Color: "+String(VDPColor[backdropColor])+"(" + String(backdropColor)+")");
  writeRegisterByteVDP(backdropColor, 7); // Transparent Text & backdropColor

  delay(2000);
}
