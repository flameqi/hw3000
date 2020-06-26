#include "hw3k_config.h"
#include "hw3k.h"
#include "printf.h"

hw hw(10, 8, 2);
data_t _data_buf_t, _data_buf_r;
mode_t mode;
typedef enum
{
  RFLR_STATE_IDLE,
  RFLR_STATE_RX_INIT,
  RFLR_STATE_RX_RUNNING,
  RFLR_STATE_RX_DONE,

  RFLR_STATE_RX_ACK_INIT,
  RFLR_STATE_RX_ACK_DONE,
  RFLR_STATE_RX_TIMEOUT,

  RFLR_STATE_TX_INIT,
  RFLR_STATE_TX_RUNNING,
  RFLR_STATE_TX_DONE,
  RFLR_STATE_TX_TIMEOUT,

  RFLR_STATE_TX_ACK_INIT,
  RFLR_STATE_TX_ACK_DONE,

  RFLR_STATE_SLEEP,
} tRFLRStates;
bool  EnableMaster = false;    //主从选择  true 为主  false 为从

static uint16_t TickCounter;

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";
int ret;
//uint16_t crc;
tRFLRStates RFstate;

//* Manages the master operation

void OnMaster(void)
{ //Serial.println(RFstate);
  switch (RFstate)
  {
    case RFLR_STATE_TX_INIT:


      hw.rx_disable();//退出接收状态

      delayMicroseconds(1);//等待射频退出接收进入IDLE状态

      _data_buf_t.data[0] = 7; //第一个字节必须为所发数据包长度   本例中我们所发数据包长度为7个字节，其中第一个字节是长度,最后2个字节是CRC校验
      _data_buf_t.len = _data_buf_t.data[0];//数据包长度(第一字节+有效数据+2字节CRC)
      _data_buf_t.data[1] = 'j';
      _data_buf_t.data[2] = 'I';
      _data_buf_t.data[3] = 'N';
      _data_buf_t.data[4] = 'G';
      //crc = hw. cal_crc_ccitt(_data_buf_t.data, _data_buf_t.len - 2);
      //_data_buf_t.data[_data_buf_t.len - 2] = crc >> 8;//包后两个字节装数据包CRC校验
      //_data_buf_t.data[_data_buf_t.len - 1] = crc;

      //IF_SERIAL_DEBUG(printf("inited!"));
      RFstate = RFLR_STATE_TX_RUNNING;
      break;

    case RFLR_STATE_TX_RUNNING:
      if (hw.tx_data(mode, &_data_buf_t) == 0)
       // printf("sended!%s\r\n",_data_buf_t.data);
        
      {
        printf("sended!%s\r\n",_data_buf_t.data);
        RFstate = RFLR_STATE_RX_INIT;
      }
      else
      {
        IF_SERIAL_DEBUG(printf("send failed!\r\n"));
        RFstate = RFLR_STATE_TX_INIT;
      }

      break;

    case RFLR_STATE_RX_INIT:

      hw.rx_enable();

      TickCounter = 0;
      RFstate = RFLR_STATE_RX_RUNNING;

      break;

    case RFLR_STATE_RX_RUNNING:

      if (hw.rx_avalible()) {
       //printf("out...............................................!%i\r\n",TickCounter);

        if (hw.rx_task(&_data_buf_r) == 0) //接收到正确的数据包
        {
          printf("rx_data!%s\r\n",_data_buf_r.data);
          RFstate = RFLR_STATE_RX_DONE;
        }
        else
        {
          printf("retry_____rxed!%s\r\n",_data_buf_r.data);
          RFstate = RFLR_STATE_TX_INIT;
        }
      }

      if (TickCounter > 800) //接收500ms超时
      {
        printf("out........................!%s\r\n",_data_buf_r.data);
        RFstate = RFLR_STATE_RX_TIMEOUT;
      }
      break;

    case RFLR_STATE_RX_DONE:

      // LedToggle();//LED闪烁
printf("rx_done data:%s...............................................!%i\r\n",_data_buf_r.data);
      RFstate = RFLR_STATE_TX_INIT;
      break;

    case RFLR_STATE_RX_TIMEOUT:

      RFstate = RFLR_STATE_TX_INIT;

      break;

    default: break;

  }

}

// Manages the slave operation

void OnSlave( void )
{ //Serial.println(RFstate);

  switch (RFstate)
  {

    case RFLR_STATE_RX_INIT:

      hw.rx_enable();

      RFstate = RFLR_STATE_RX_RUNNING;

      break;


    case RFLR_STATE_RX_RUNNING:

      if (hw.rx_avalible()) {
      // printf("on salve  rfstate:%x.......................................==\r\n", RFstate);
        ret =hw.rx_task(&_data_buf_r);
        if (ret == 0) //接收到正确的数据包
        {

          printf("................rxdata:%s\r\n", _data_buf_r.data)  ;
          RFstate = RFLR_STATE_TX_ACK_INIT;
        }
        else
        {
          printf(".......................rs_task_ret=%i\r\n",ret);
          printf("retry rxdata:%s\r\n", _data_buf_r.data)  ;
          RFstate = RFLR_STATE_RX_RUNNING;
        }
      }
      break;


    case RFLR_STATE_TX_ACK_INIT:

      hw.rx_disable();//退出接收状态
      HAL_Delay_nMS(1);//等待射频退出接收进入IDLE状态

      _data_buf_t.data[0] = 7; //第一个字节必须为所发数据包长度   本例中我们所发数据包长度为7个字节，其中第一个字节是长度,最后2个字节是CRC校验
      _data_buf_t.len = _data_buf_t.data[0];//数据包长度(第一字节+有效数据+2字节CRC)
      _data_buf_t.data[1] = 'w';
      _data_buf_t.data[2] = 'O';
      _data_buf_t.data[3] = 'N';
      _data_buf_t.data[4] = 'G';
      //crc = hw.cal_crc_ccitt(_data_buf_t.data, _data_buf_t.len - 2);
      //_data_buf_t.data[_data_buf_t.len - 2] = crc >> 8;//包后两个字节装数据包CRC校验
      //_data_buf_t.data[_data_buf_t.len - 1] = crc;


      RFstate = RFLR_STATE_TX_RUNNING;

      break;

    case RFLR_STATE_TX_RUNNING:

      if (hw.tx_data(mode, &_data_buf_t) == 0)
      {

        printf("tx_data:%s\r\n", _data_buf_t.data)  ;

        RFstate = RFLR_STATE_RX_INIT;
      }
      else
      {
        RFstate = RFLR_STATE_TX_INIT;
      }

      break;

    default: break;
  }
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  printf_begin();


  mode.osc  = OSC20MHZ;
  mode.band = FREQ_BAND_433MHZ;//433模块  868模块选择
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

  if (EnableMaster)
  {
    RFstate = RFLR_STATE_TX_INIT;
  }
  else
  {
    RFstate = RFLR_STATE_RX_INIT;
  }



}

void loop() {

  // put your main code here, to run repeatedly:

  if (EnableMaster == true)
  {

    OnMaster();
  }
  else
  {
    OnSlave();
  }
TickCounter++;
}
