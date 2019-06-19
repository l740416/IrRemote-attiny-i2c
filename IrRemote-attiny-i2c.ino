#include <Arduino.h>
#include <IRremote.h>
#include <Wire.h>

// Firmware version
#define VERSION_MAJOR                  0
#define VERSION_MINOR                  1

// General define
#define TRUE                           1
#define FALSE                          0

// I2C general
#define SLAVE_ADDRESS                  0x05

#if defined(__AVR_ATtiny85__)
  #define LED1_PIN                       4
  #define LED2_PIN                       4
  #define BTN_PIN                        0  // not supported
  #define SEND_PIN                       1
  #define RECV_PIN                       3
#elif defined(__AVR_ATtiny1634__)
  #define LED1_PIN                       PIN_C0
  #define LED2_PIN                       PIN_C4
  #define BTN_PIN                        PIN_C2
  #define SEND_PIN                       PIN_B2
  #define RECV_PIN                       PIN_A0
#else
  #error Only Attiny85 and Attiny1634 are supported
#endif

IRsend irsend;
IRrecv irrecv(RECV_PIN, LED2_PIN);
decode_results results;

// I2C read commmands
#define CMD_READ_COMMAND_FIRST         0x00
#define CMD_INFO                       0x00   // Out:4B
#define CMD_GET_CODETYPE               0x01   // Out:4B
#define CMD_GET_CODEVALUE              0x02   // Out:4B
#define CMD_GET_CODELENGTH             0x03   // Out:4B
#define CMD_GET_BUFFER_KHZ_LEN         0x04
#define CMD_GET_BUFFER                 0x05
#define CMD_READ_COMMAND_LAST         (0x05 + RAWBUF/2)

// I2C write commmands
#define CMD_WRITE_COMMAND_FIRST        0x80   // Must be > (0x05 + RAWBUF/2)
#define CMD_SET_CODETYPE               0x80   // In :4B
#define CMD_SET_CODEVALUE              0x81   // In :4B
#define CMD_SET_CODELENGTH             0x82   // In :4B
#define CMD_WRITE_COMMAND_LAST         0x82

// I2C action commmands
#define CMD_ACTION_COMMAND_FIRST       0x90
#define CMD_RECV                       0x90
#define CMD_SEND                       0x91
#define CMD_SEND_RAW                   0x92
#define CMD_STORE_RAW                  0x93
#define CMD_STOP_STORE_RAW             0x94
#define CMD_ACTION_COMMAND_LAST        0x94

// I2C buffer write commmands
#define CMD_BUFWRITE_COMMAND_FIRST     0xA0
#define CMD_WRITE_KHZ_LEN              0xA0
#define CMD_WRITE_BUFFER               0xA1
#define CMD_BUFWRITE_COMMAND_LAST     (0xA1 + RAWBUF/2)

// I2C input values
struct input_t
{
  int32_t  codeType;
  uint32_t codeValue;
  int32_t  codeLen;
};

// I2C output values
struct output_t
{
  uint8_t  version;
  uint8_t  rawBufSize;
  uint16_t recvCount;
  uint32_t codeType;
  uint32_t codeValue;
  int32_t  codeLen;

  // below data is input/output
  uint16_t kHz;
  uint16_t rawCodeLen;
  uint16_t rawCodes[RAWBUF];
};

// I2C action flags
struct action_t
{
  uint8_t  doRecv;
  uint8_t  doSend;
  uint8_t  doSendRaw;
  uint8_t  doStoreRaw;
  uint8_t  doStopStoreRaw;
};

input_t  inputData = {0, 0, 0};
output_t outputData;
action_t actionData = {FALSE, FALSE};

// Global veriables
#define MAX_RECV_BYTES 6
byte receivedCommands[MAX_RECV_BYTES];
void* requestPtr = &(outputData.version);
byte requestCommand = CMD_INFO;


void setup()
{
  outputData.version = ((uint8_t)VERSION_MAJOR << 4) | VERSION_MINOR;
  outputData.rawBufSize = RAWBUF;
  outputData.recvCount = 0;  

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(SEND_PIN, OUTPUT);
  
  digitalWrite(LED1_PIN, HIGH);
  delay(1000);;
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, HIGH);
  delay(1000);;
  digitalWrite(LED2_PIN, LOW);
  delay(1000);

  irrecv.enableIRIn();
  irrecv.resume();
}

void loop() { 

  if (actionData.doRecv == TRUE)
  {  
    irrecv.enableIRIn();
    irrecv.resume();
    actionData.doRecv = FALSE;
  }
  else if (actionData.doSend == TRUE)
  {
    sendCode();
    digitalWrite(LED1_PIN, HIGH);
    delay(100);
    digitalWrite(LED1_PIN, LOW);
    delay(100);
    digitalWrite(LED1_PIN, HIGH);
    delay(100);
    digitalWrite(LED1_PIN, LOW);
    actionData.doSend = FALSE;
  }
  else if (actionData.doSendRaw == TRUE)
  {
    irsend.sendRaw(outputData.rawCodes, outputData.rawCodeLen, outputData.kHz);
    digitalWrite(LED1_PIN, HIGH);
    delay(200);
    digitalWrite(LED1_PIN, LOW);
    delay(200);
    digitalWrite(LED1_PIN, HIGH);
    delay(200);
    digitalWrite(LED1_PIN, LOW);
    actionData.doSendRaw = FALSE;
  }
  else if (actionData.doStopStoreRaw == TRUE)
  {
    actionData.doStoreRaw = FALSE;
    actionData.doStopStoreRaw = FALSE;
    digitalWrite(LED1_PIN, HIGH);
    delay(100);
    digitalWrite(LED1_PIN, LOW);
  }

  if (irrecv.decode(&results))
  {
    outputData.recvCount += 1;
    storeCode(&results);

    if (actionData.doStoreRaw == TRUE)
    {
      storeRaw(&results);
      actionData.doStoreRaw = FALSE;
    }

    digitalWrite(LED1_PIN, HIGH);
    delay(500);
    digitalWrite(LED1_PIN, LOW);

    irrecv.resume();
  }
}

void receiveEvent(uint8_t bytesReceived) 
{  
  for(uint8_t i = 0; i < bytesReceived; i++)
  {
    if(i < MAX_RECV_BYTES)
    {
      receivedCommands[i] = Wire.read();
    }
    else
    {
      // if we receive more data then allowed just throw it away
      char c = Wire.read();
    }    
  }

  if(receivedCommands[0] >= CMD_READ_COMMAND_FIRST && receivedCommands[0] <= CMD_READ_COMMAND_LAST)
  {
    requestCommand = receivedCommands[0] - CMD_READ_COMMAND_FIRST;
  }
  else if(receivedCommands[0] >= CMD_WRITE_COMMAND_FIRST && receivedCommands[0] <= CMD_WRITE_COMMAND_LAST)
  {
    uint8_t* writePtr = (uint8_t*)((uint32_t*)(&inputData) + (receivedCommands[0] - CMD_WRITE_COMMAND_FIRST));
    *(writePtr    ) = receivedCommands[1];
    *(writePtr + 1) = receivedCommands[2];
    *(writePtr + 2) = receivedCommands[3];
    *(writePtr + 3) = receivedCommands[4];
  }
  else if(receivedCommands[0] >= CMD_BUFWRITE_COMMAND_FIRST && receivedCommands[0] <= CMD_BUFWRITE_COMMAND_LAST)
  {
    uint8_t* writePtr = (uint8_t*)((uint32_t*)(&(outputData.kHz)) + (receivedCommands[0] - CMD_BUFWRITE_COMMAND_FIRST));
    *(writePtr    ) = receivedCommands[1];
    *(writePtr + 1) = receivedCommands[2];
    *(writePtr + 2) = receivedCommands[3];
    *(writePtr + 3) = receivedCommands[4];
  }
  else if(receivedCommands[0] >= CMD_ACTION_COMMAND_FIRST && receivedCommands[0] <= CMD_ACTION_COMMAND_LAST)
  {
    uint8_t* actionPtr = (uint8_t*)(&actionData) + (receivedCommands[0] - CMD_ACTION_COMMAND_FIRST);
    *actionPtr = TRUE;
  }
}

void requestEvent()
{
  uint8_t* ptr = ((uint8_t *)(&(outputData))) + ((requestCommand - CMD_READ_COMMAND_FIRST) << 2);
  Wire.write(ptr, 4);
}

void storeCode(decode_results *results)
{
  outputData.codeType = results->decode_type;
  outputData.codeValue = results->value;
  outputData.codeLen = results->bits;
}

void storeRaw(decode_results *results)
{
  outputData.rawCodeLen = results->rawlen - 1;
  for (int i = 1; i <= outputData.rawCodeLen; i++)
  {
    if (i % 2)
    {
      // Mark
      outputData.rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
    } 
    else
    {
      // Space
      outputData.rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
    }
  }
}

void sendCode()
{
  if (inputData.codeType == NEC)
  {
    irsend.sendNEC(inputData.codeValue, inputData.codeLen);
  } 
  else if (inputData.codeType == SONY)
  {
    irsend.sendSony(inputData.codeValue, inputData.codeLen);
  } 
  else if (inputData.codeType == SAMSUNG)
  {
    irsend.sendSAMSUNG(inputData.codeValue, inputData.codeLen);
  } 
  /*
  else if (inputData.codeType == PANASONIC)
  {
    irsend.sendPanasonic(inputData.codeValue, inputData.codeLen);
  } 
  */
  else if (inputData.codeType == LG)
  {
    irsend.sendLG(inputData.codeValue, inputData.codeLen);
  } 
}
