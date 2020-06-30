#include "hw3k_config.h"
#include "hw3k.h"
#include "printf.h"
#include "fun.h"
#include <MsTimer2.h>
#define RERX 600
#define LEN 10
hw hw(10, 8, 2);
data_t _data_buf_t, _data_buf_r;
mode_t mode;
bool role = true, rxok = false;
//static uint16_t TickCounter;
unsigned long time, time_rx;
const uint16_t IDTable[] = {0x0001, 0x0002, 0x0003, 0x0004};
uint16_t ID;
int idIndex = 0, tick = 0;
uint8_t PingMsg[LEN];
uint8_t PongMsg[LEN];
int switchb = 1;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  printf_begin();

  mode.osc = OSC20MHZ;
  mode.band = FREQ_BAND_433MHZ; //433模块  868模块选择
  mode.rate = SYMBOL_RATE_10K;
  mode.freq_mode = DIRECT;
  mode.ch = 1; //信道设置，_mode.band=FREQ_BAND_433MHZ且_mode.freq_mode=DEFAULT模式下有效，设置范围0-75
  //433模块频点设置范围430.00MHZ-460.00MHZ
  // 868模块频点设置范围860.00MHZ-900.00MHZ
  mode.freq = 433.33; //频点设置，_mode.freq_mode=DIRECT模式下有效
  mode.power = 20;
  mode.frame_mode = FRAME;
  mode.ack_mode = DISABLE;
  mode.lp_enable = DISABLE;
  //attachInterrupt(0, intrrupt, RISING);
  
  hw.init(mode);
  MsTimer2::set(5000, switchblink); // 中断设置函数，每 500ms 进入一次中断
  MsTimer2::start();
}
void switchblink()
{
  if (switchb == 0)
  {
   switchb= 1;
  }
  else
  {
    switchb =0;
  }
}
void coding(uint8_t *res, data_t *tar, uint8_t len)
{
  if (len > 252)
    return;
  tar->len = len + 3;
  tar->data[0] = tar->len;

  for (int i = 0; i < (tar->len - 3); i++)
  {
    tar->data[i + 1] = res[i];
  }
}
void decoding(data_t *res, uint8_t *tar)
{
  for (int i = 0; i < res->len - 3; i++)
  {
    tar[i] = res->data[i + 1];
  }
}
void loop()
{

  // put your main code here, to run repeatedly:
  time = micros();
  /*  int bit = 24;
  for (int i = 3; i >= 0; i--)
  {
    unsigned long temp = time << bit;
    PingMsg[i] = temp >> 24;
    bit -= 8;
  }*/
  //ID_r=IDTable[0];
  if (idIndex >= (sizeof(IDTable) / 2))
  {
    idIndex = 0;
  }

  PingMsg[5] = time;
  PingMsg[4] = time >> 8;
  PingMsg[3] = time >> 16;
  PingMsg[2] = time >> 24;
  PingMsg[1] = IDTable[idIndex];
  PingMsg[0] = IDTable[idIndex] >> 8;
  PingMsg[8]=switchb;
  coding(PingMsg, &_data_buf_t, sizeof(PingMsg));
  hw.rx_disable();
  //delayMicroseconds(10);
  if (hw.tx_data(mode, &_data_buf_t) == 0)
  {
    //delayMicroseconds(2);
    //printf("\r\n");
    //printf("id:%x,tx_time:%ld\r\n", IDTable[idIndex], time);
    // hw.rx_disable();
    //printf("...........................................pingmsg8:%d\r\n",PingMsg[8]);
    hw.rx_enable();
    delayMicroseconds(1);
    while (1)
    {
      if (tick <= RERX)
      {
        if (hw.rx_avalible())
        {

          tick = 0;
          break;
        }
      }
      else
      {
        tick = 2;
        break;
      }
      //
      tick++;
    }
    //printf("..........................tick:%i\r\n", tick);
    if (tick <= 1)
    {
      if (hw.rx_task(&_data_buf_r) == 0)
      {

        decoding(&_data_buf_r, PongMsg);
        ID = PongMsg[0] << 8 | PongMsg[1];
        time_rx = PongMsg[5];
        time_rx = time_rx | ((unsigned long)PongMsg[4] << 8);
        time_rx = time_rx | ((unsigned long)PongMsg[3] << 16);
        time_rx = time_rx | ((unsigned long)PongMsg[2] << 24);
        printf("I receive ID:%x,receive time:%ld\r\n", ID, time_rx);
        if (ID == 0x0003)
        {

          printf(".......light:%d,temperature:%d,humidity:%d\r\n", PongMsg[8], PongMsg[6], PongMsg[7]);
        }
        //idIndex++;
      }
    }
  }
  idIndex++;
}