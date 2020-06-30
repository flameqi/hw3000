#include <SimpleDHT.h>

//#include <DHT.h>

#include "hw3k_config.h"
#include "hw3k.h"
#include "printf.h"
#include "fun.h"
#define DHTPIN 3
#define SWITCHPIN 4
//#define DHTTYPE DHT11
#define LEN 10
hw hw(10, 8, 2);
//DHT dht(DHTPIN, DHTTYPE);
SimpleDHT11 dht11(DHTPIN);
int temp;
data_t _data_buf_t, _data_buf_r;
mode_t mode;

static uint16_t TickCounter;
unsigned long time, time_tx;
uint16_t ID = 0x0003, ID_r;
uint8_t PingMsg[LEN];
uint8_t PongMsg[LEN];

void setup()
{
  pinMode(A0, INPUT);
  pinMode(SWITCHPIN, INPUT);
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
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    //Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
    //return;
    temperature=0;
    humidity=0;
  }
  

  //int h =(int) dht.readHumidity()*100;
  //int t =(int) dht.readTemperature();
  time = micros();
  // put your main code here, to run repeatedly:
 
  // printf("data:%s,,,,res:%s,,,,,,,len:%i\r\n", _data_buf_t.data, PingMsg, _data_buf_t.len);
  PingMsg[5] = time;
  PingMsg[4] = time >> 8;
  PingMsg[3] = time >> 16;
  PingMsg[2] = time >> 24;
  PingMsg[7]=humidity;
  PingMsg[6]=temperature;
  PingMsg[8] = analogRead(A0);
  PingMsg[1] = ID;
  PingMsg[0] = ID >> 8; 
  coding(PingMsg, &_data_buf_t, sizeof(PingMsg));
  //Float_to_Byte(temp, PingMsg, 6);
  //hw.rx_disable();
  hw.rx_enable();
  delayMicroseconds(1);
  while (1)
  {
    if (hw.rx_avalible())
    {
      break;
    }
  }

  if (hw.rx_task(&_data_buf_r) == 0)
  {

    decoding(&_data_buf_r, PongMsg);
    ID_r = PongMsg[0] << 8 | PongMsg[1];
    time_tx = PongMsg[5];
    time_tx = time_tx | ((unsigned long)PongMsg[4] << 8);
    time_tx = time_tx | ((unsigned long)PongMsg[3] << 16);
    time_tx = time_tx | ((unsigned long)PongMsg[2] << 24);

    if (ID_r == ID)
    {
      printf("hit me myID:%x,rx_time:%ld,SWITCHPIN::%d\r\n", ID_r, time_tx,PongMsg[8]);
    if (PongMsg[8]==1){digitalWrite(SWITCHPIN,HIGH);}else{digitalWrite(SWITCHPIN,LOW);}
      hw.rx_disable();
      delayMicroseconds(1);
      if (hw.tx_data(mode, &_data_buf_t) == 0)
      {//t=27.6f;
        printf("I sened!! my ID_R:%x,sened_data:%ld,temperature:%d,humidity:%d\r\n", ID, time,temperature,humidity);
      }
    }
    //printf("rx_data:%s len:%i\r\n", _data_buf_r.data,_data_buf_r.len);
  }
}
