#include "hw3k_config.h"
#include "hw3k.h"
#include "printf.h"

hw hw(10, 8, 2);
data_t _data_buf_t, _data_buf_r;
mode_t mode;
bool role = false;
bool rxok=false;
static uint16_t TickCounter;
unsigned long time;
uint8_t thisName[]="one";
uint8_t PingMsg[];
uint8_t PongMsg[];

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
}
void coding(uint8_t *res, data_t *tar, uint8_t len)
{
  if (len > 252)
    return;
  tar->len = len+3;
  tar->data[0] = tar->len;

  for (int i = 0; i < (tar->len-3); i++)
  {
    tar->data[i + 1] = res[i];
  }
}
void decoding(data_t *res , uint8_t *tar){
  for(int i=0; i<res->len-3;i++){
    tar[i]=res->data[i+1];
  }
}

void loop()
{

  // put your main code here, to run repeatedly:
  coding(PingMsg, &_data_buf_t, sizeof(PingMsg));
  // printf("data:%s,,,,res:%s,,,,,,,len:%i\r\n", _data_buf_t.data, PingMsg, _data_buf_t.len);

  if (role)
  {
    hw.rx_disable();
    delayMicroseconds(1);
    if (hw.tx_data(mode, &_data_buf_t) == 0)
    {
      printf("sened!!");
    }
  }
  else
  {
    hw.rx_enable();
    delayMicroseconds(1);
    if (hw.rx_avalible())
    {
      if (hw.rx_task(&_data_buf_r) == 0)
      {
        rxok=true;
        //printf("rx_data:%s len:%i\r\n", _data_buf_r.data,_data_buf_r.len);
      }
      else{
        rxok=false;
        //printf("rx_failed!!!\r\n");
      }
    }
    else{rxok=false;}
  }
  if (rxok){
    decoding(&_data_buf_r,PongMsg);
    time=PongMsg[0];
    time=time|((unsigned long )PongMsg[1]<<8);
    time=time|((unsigned long )PongMsg[2]<<16);
    time=time|((unsigned long )PongMsg[3]<<24);
    printf("rx_time:%ld,,p0=%x,p1=%x,p2=%x,p3=%x,\r\n",time,PongMsg[0],PongMsg[1],PongMsg[2],PongMsg[3]);
  }
}
