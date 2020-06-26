#include "hw3k_config.h"
#include "hw3k.h"
#include "printf.h"

hw hw(10, 8, 2);
data_t _data_buf_t, _data_buf_r;
mode_t mode;
bool role = true;
static uint16_t TickCounter;

uint8_t PingMsg[] = "PINGG";
uint8_t PongMsg[] = "PONG";

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
void copystr(uint8_t *res, data_t *tar, uint8_t len)
{
  if (len > 252)
    return;
  tar->len = len;
  tar->data[0] = tar->len;

  for (int i = 0; i < tar->len; i++)
  {
    tar->data[i + 1] = res[i];
  }

  res[3] = 'p';
}

void loop()
{

  // put your main code here, to run repeatedly:
  copystr(PingMsg, &_data_buf_t, sizeof(PingMsg));
  // printf("data:%s,,,,res:%s,,,,,,,len:%i\r\n", _data_buf_t.data, PingMsg, _data_buf_t.len);

  if (role)
  {
    hw.rx_disable();
    delayMicroseconds(1);
    if (hw.tx_data(mode, &_data_buf_t) == 0)
    {
      printf("sened!!\r\n");
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
        printf("rx_data:%s\r\n", _data_buf_r.data);
      }
    }
  }
}
