
#include "_reg_hw3k.h"

#include "hw3k_config.h"
#include "hw3k.h"


hw::hw(uint8_t csnPin, uint8_t pdnPin, uint8_t irqPin)
  : csnPin(csnPin), pdnPin(pdnPin), irqPin(irqPin), csDelay(20)
{
  pinMode(csnPin, OUTPUT);
  pinMode(pdnPin, OUTPUT);
  pinMode(irqPin, INPUT);

  _SPI.setBitOrder(MSBFIRST);
  _SPI.setDataMode(SPI_MODE1);
  _SPI.setClockDivider(SPI_CLOCK_DIV8);
  _SPI.begin();
//  IF_SERIAL_DEBUG(printf("spi be \r\n" ));

}

void hw::init(mode_t mode)
{
  uint8_t i;
  uint16_t agc_table[16] = {0x1371, 0x1351, 0x0B51, 0x2F71, 0x2F51, 0x2E51,
                            0x2E31, 0x4B31, 0x4731, 0x4631, 0x4531, 0x4431,
                            0x6131, 0x6031, 0x6011, 0x6009
                           };

  uint16_t reg_val;
  _mode = mode;
  power_down();
  HAL_Delay_nMS(1);
  power_up();


  write_reg(0x4C, 0x5555);   //Bank 0 open by default

  while (!(read_reg(INTFLAG) & 0x4000)); //wait for chip ready

  write_reg(0x4C, 0x55AA); //Bank 1 open

  for (i = 0; i < 16; i++) {
    write_reg(0x1B + i, agc_table[i]);
  }
  write_reg(0x03, 0x0508);
  write_reg(0x11, 0xC630);

  switch (mode.rate) {
    case SYMBOL_RATE_1K:
      write_reg(0x14, 0x1935);

      write_reg(0x40, 0x0008);
      write_reg(0x41, 0x0010);
      write_reg(0x42, 0x82D8);
      write_reg(0x43, 0x3D38);

      break;
    case SYMBOL_RATE_10K:
      write_reg(0x14, 0x1935);

      write_reg(0x40, 0x0008);
      write_reg(0x41, 0x0010);
      write_reg(0x42, 0x82D8);
      write_reg(0x43, 0x3D38);
      break;
    case SYMBOL_RATE_19K2:
      write_reg(0x14, 0x1915);
      break;
    case SYMBOL_RATE_38K4:
      write_reg(0x14, 0x1915);
      break;
    case SYMBOL_RATE_50K:
      write_reg(0x14, 0x1915);
      break;
    case SYMBOL_RATE_100K:
      write_reg(0x14, 0x1915);
      break;
    default:
      break;
  }

  switch (mode.band) {
    case FREQ_BAND_315MHZ:
      write_reg(0x17, 0xF223);
      break;
    case FREQ_BAND_433MHZ:
      write_reg(0x17, 0xF6C2);
      break;
    case FREQ_BAND_779MHZ:
      write_reg(0x17, 0xFB61);
      break;
    case FREQ_BAND_868MHZ:
      write_reg(0x17, 0xFB61);
      break;
    case FREQ_BAND_915MHZ:
      write_reg(0x17, 0xFB61);
      break;
    default:
      break;
  }

  write_reg(0x51, 0x001B);
  write_reg(0x55, 0x8003);
  write_reg(0x56, 0x4155);
  write_reg(0x62, 0x70ED);

  write_reg(0x4C, 0x5555);  //Bank 0 open
  write_reg(INTIC, 0x8000); //clear por int

  write_reg(MIXFW, 0x2E35);  //must modify this value
  write_reg(MODECTRL, 0x100F); //must close GPIO clock for noise reason

  /*osc set*/
  if (mode.osc == OSC20MHZ)
  {
    write_reg(MODEMCTRL, 0x5201); //20MHz osc select
  }
  else
  {
    write_reg(MODEMCTRL, 0x1201); //26MHz osc select
  }

  /*frequency set*/
  if (mode.freq_mode == DEFAULT_T && mode.band == FREQ_BAND_433MHZ)
  {
    write_reg(RFCFG, 0x3312);
    write_reg(FREQCFG0, 0x50CC); //default mode only for 433MHz  ch_space 400K

    write_reg(CHCFG0, (mode.ch & 0x00FF) << 8); //channel1

  }
  else
  {
    freq_set(mode); //from deep sleep/sleep need reconfigure this reg
  }

  /*data rate*/
  rate_set(mode);

  /*tx power set*/
  power_set(mode);

  /*rx power set*/
  if (mode.lp_enable == ENABLE) { //HOP_TIMER/(HOP_TIMER+LP_TIMER)
    reg_val = read_reg(MODEMCTRL);
    reg_val &= 0xF0FF;
    reg_val |= 0x0E00;  //LP_TIMER set
    reg_val |= 0x0004;  //LP_ENABLE
    write_reg(MODEMCTRL, reg_val);
    reg_val = read_reg(HOPCFG);
    reg_val &= 0xFF0F;
    reg_val |= 0x0080;  //HOP_TIMER set
    write_reg(HOPCFG, reg_val);
  }

  /*frame set*/
  frame_set(mode);

  // write_reg(0x02, 0xc006);
  write_reg(0x14, 0x7e7e);//set SFD First word
  write_reg(0x15, 0x7e7e);//set SFD Second word
  //write_reg(PKTCFG1,0x01FF); //����ͬ���ִ������Ϊ0

  //IF_SERIAL_DEBUG(printf("init end ........................................... \r\n" ));
  _state = IDLE;
}
uint8_t hw::getirqPin(void) {
  return irqPin;
}
/******************************************************************************
   @brief    freq_set

   @note

   @param  mode,freq

   @retval   None

   @version  1.0
   @date     2016-01-14
   @author   sundy

  433ģ��Ƶ�����÷�Χ430.00MHZ-460.00MHZ
  868ģ��Ƶ�����÷�Χ860.00MHZ-900.00MHZ

 ******************************************************************************/
void hw::freq_set(mode_t mode)
{
  uint16_t RF_FREQ_BASE, reg;
  uint32_t RF_FREQ_FRACTION;
  float freq = mode.freq;

  switch (mode.band) {

    case FREQ_BAND_433MHZ:

      RF_FREQ_BASE = (uint16_t)floor(freq * 4 / 20);
      write_reg(RFCFG, 0x3312);
      RF_FREQ_FRACTION = (uint32_t)round((freq * 4 / 20 - RF_FREQ_BASE) * pow(2, 21));
      write_reg(FREQCFG1, 0x8000 | (RF_FREQ_FRACTION >> 16));
      write_reg(FREQCFG2, (uint16_t)RF_FREQ_FRACTION); //must first set
      reg = read_reg(FREQCFG0) & 0xE000;
      write_reg(FREQCFG0, reg | RF_FREQ_BASE); //last set RF_FREQ_BASE

      break;

    case FREQ_BAND_868MHZ:

      write_reg(RFCFG, 0x1312);
      RF_FREQ_BASE = (uint16_t)floor(freq * 2 / 20);
      RF_FREQ_FRACTION = (uint32_t)round((freq * 2 / 20 - RF_FREQ_BASE) * pow(2, 21));
      write_reg(FREQCFG1, 0x8000 | (RF_FREQ_FRACTION >> 16));
      write_reg(FREQCFG2, (uint16_t)RF_FREQ_FRACTION); //must first set
      reg = read_reg(FREQCFG0) & 0xE000;
      write_reg(FREQCFG0, reg | RF_FREQ_BASE); //last set RF_FREQ_BASE

      break;

    default: break;

  }
}

/******************************************************************************
   @brief    rate_set

   @note

   @param  mode

   @retval   None

   @version  1.0
   @date     2016-01-14
   @author   sundy
 ******************************************************************************/
void hw::rate_set(mode_t mode)
{
  if (mode.osc == OSC20MHZ) {
    switch (mode.rate) {
      case SYMBOL_RATE_1K:
        write_reg(SYMRATE0, 0x0008);
        write_reg(SYMRATE1, 0x0031);
        write_reg(FILTERBAND, 0x0002);

        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x0007);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x0007);
            break;
          case FREQ_BAND_779MHZ:
            write_reg(DEVIATION, 0x0002);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0002);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0002);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_10K:
        write_reg(SYMRATE0, 0x0051);
        write_reg(SYMRATE1, 0x00EC);
        write_reg(FILTERBAND, 0x0025);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x007B);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          case FREQ_BAND_779MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_19K2:
        write_reg(SYMRATE0, 0x009D);
        write_reg(SYMRATE1, 0x0049);
        write_reg(FILTERBAND, 0x002A);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x007B);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          case FREQ_BAND_779MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_38K4:
        write_reg(SYMRATE0, 0x013A);
        write_reg(SYMRATE1, 0x0093);
        write_reg(FILTERBAND, 0x0033);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x007B);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          case FREQ_BAND_779MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_50K:
        write_reg(SYMRATE0, 0x0199);
        write_reg(SYMRATE1, 0x009A);
        write_reg(FILTERBAND, 0x0037);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x007B);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          case FREQ_BAND_779MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0029);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_100K:
        write_reg(SYMRATE0, 0x0333);
        write_reg(SYMRATE1, 0x0033);
        write_reg(FILTERBAND, 0x0078);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x00F6);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x00A4);
            break;
          case FREQ_BAND_779MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0052);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  } else {
    switch (mode.rate) {
      case SYMBOL_RATE_1K:
        write_reg(SYMRATE0, 0x0008);
        write_reg(SYMRATE1, 0x0031);
        write_reg(FILTERBAND, 0x0002);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x0006);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x0004);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0002);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0002);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_10K:
        write_reg(SYMRATE0, 0x0051);
        write_reg(SYMRATE1, 0x00EC);
        write_reg(FILTERBAND, 0x0025);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x005F);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x003F);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_19K2:
        write_reg(SYMRATE0, 0x009D);
        write_reg(SYMRATE1, 0x0049);
        write_reg(FILTERBAND, 0x002A);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x005F);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x003F);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_38K4:
        write_reg(SYMRATE0, 0x013A);
        write_reg(SYMRATE1, 0x0093);
        write_reg(FILTERBAND, 0x0033);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x005F);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x003F);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_50K:
        write_reg(SYMRATE0, 0x0199);
        write_reg(SYMRATE1, 0x009A);
        write_reg(FILTERBAND, 0x0037);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x005F);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x003F);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x0020);
            break;
          default:
            break;
        }
        break;
      case SYMBOL_RATE_100K:
        write_reg(SYMRATE0, 0x0333);
        write_reg(SYMRATE1, 0x0033);
        write_reg(FILTERBAND, 0x0078);
        switch (mode.band) {
          case FREQ_BAND_315MHZ:
            write_reg(DEVIATION, 0x00BD);
            break;
          case FREQ_BAND_433MHZ:
            write_reg(DEVIATION, 0x007E);
            break;
          case FREQ_BAND_868MHZ:
            write_reg(DEVIATION, 0x003F);
            break;
          case FREQ_BAND_915MHZ:
            write_reg(DEVIATION, 0x003F);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

void hw::power_set(mode_t mode)
{
  switch (mode.power) {
    case -40:
      write_reg(PACFG, 0x003F);
      break;
    case -16:
      write_reg(PACFG, 0x013F);
      break;
    case -10:
      write_reg(PACFG, 0x023F);
      break;
    case -5:
      write_reg(PACFG, 0x043F);
      break;
    case 0:
      write_reg(PACFG, 0x073F);
      break;
    case 5:
      write_reg(PACFG, 0x0C3F);
      break;
    case 10:
      write_reg(PACFG, 0x183F);
      break;
    case 15:
      write_reg(PACFG, 0x2A3F);
      break;
    case 18:
      write_reg(PACFG, 0x643F);
      break;
    case 20:
      write_reg(PACFG, 0xFF3F);
      break;
    default:
      break;
  }
}

/******************************************************************************
   @brief    frame_set

   @note

   @param   mode

   @retval   None

   @version  1.0
   @date     2016-01-14
   @author   sundy
 ******************************************************************************/
void hw::frame_set(mode_t mode)
{

  if (mode.frame_mode == FRAME)
  { //frame mode
    write_reg(PKTCTRL, 0xC000);
    write_reg(PKTCFG0, 0x8010); //SFD 4bytes, preamble 16 bytes
    write_reg(PREACFG, 0x0206);
    if (mode.ack_mode == ENABLE)
    {
      write_reg(ACKCFG, 0x30FF); //re tx times and re ack time
      write_reg(PIPECTRL, 0x0011); //PIPE 0 enable
    }
    else
    {
      write_reg(PIPECTRL, 0x0001); //PIPE 0 enable
    }
  }
  else
  { //direct fifo mode

    write_reg(PKTCTRL, 0x4000);
    write_reg(PKTCFG0, 0x8010); //SFD 4bytes, preamble 8 bytes
    write_reg(PREACFG, 0x0206);
    write_reg(FIFOSTA, 0x0000); //first 2 bytes is length
    write_reg(LEN0RXADD, 0x0000); //no add bytes
    write_reg(PIPECTRL, 0x0001); //PIPE 0 enable
  }
}
/****************************************************************************/
void hw::csn(bool mode)
{



  digitalWrite(csnPin, mode);
  delayMicroseconds(csDelay);

}
/****************************************************************************/
inline void hw::beginTransaction()
{
#if defined(RF24_SPI_TRANSACTIONS)
  _SPI.beginTransaction(SPISettings(RF24_SPI_SPEED, MSBFIRST, SPI_MODE0));
#endif // defined(RF24_SPI_TRANSACTIONS)
  // IF_SERIAL_DEBUG(printf("spi_begin\r\n"));
  csn(LOW);
}

/****************************************************************************/

inline void hw::endTransaction()
{
  // IF_SERIAL_DEBUG(printf("spi_end\r\n"));
  csn(HIGH);
#if defined(RF24_SPI_TRANSACTIONS)
  _SPI.endTransaction();
#endif // defined(RF24_SPI_TRANSACTIONS)
}
//////////////////////////////////////////////////////////////////////////
void hw::read_fifo(uint8_t reg, uint8_t* buf, uint8_t len)
{
  uint8_t status; reg = reg & 0x7F;
  //IF_SERIAL_DEBUG(printf("read_f->reg=%x ,buf[0]=%x,buf=%s ,len=%i \r\n", reg, * buf, len ));


  beginTransaction();
  status = _SPI.transfer(reg);
  while (len--) {
    *buf++ = _SPI.transfer(0x00);
    IF_SERIAL_DEBUG(printf("read_f->reg=%x ,buf=%s ,len=%i \r\n", reg, buf, len ));
  }
  endTransaction();
}
///////////////////////////////////////////////////////////////////////////
uint16_t hw::read_reg( uint8_t reg )
{
  uint16_t ret;
  reg = reg & 0x7F;
  beginTransaction();
  _SPI.transfer(reg);
  //delayMicroseconds(40);
  ret = _SPI.transfer16(0x00) ;

  endTransaction();

  //IF_SERIAL_DEBUG(printf("read_r->reg=%x,ret=%x\r\n", reg, ret));
  return ret;
}
///////////////////////////////////////////////////////////////////////
void hw::write_fifo(uint8_t reg, const uint8_t *buf, uint8_t len)
{
  uint8_t status; reg = reg | 0x80;
  //

  beginTransaction();
  status = _SPI.transfer( reg);
  while (len--) {
    IF_SERIAL_DEBUG(printf("write_f->reg=%x ,buf=%s ,len=%i \r\n", reg, buf, len ));
    _SPI.transfer(*buf++);
  }
  endTransaction();

}
/****************************************************************************/
void hw::write_reg(uint8_t reg, const uint16_t buf )
{

  reg = reg | 0x80;
  //

  beginTransaction();
  _SPI.transfer( reg);
  //delayMicroseconds(40);

  _SPI.transfer16(buf );
  //_SPI.transfer((char)*buf);
  endTransaction();

  //IF_SERIAL_DEBUG(printf("write_r->reg=%x ,buf=%x \r\n", reg & 0x7f, buf));

}

////////////////////////////////////////////////////////////
void hw::power_down(void) {
  digitalWrite(pdnPin, HIGH);
}
void hw::power_up(void) {
  digitalWrite(pdnPin, LOW);
}

///////////////////////////////////////////////////////////

uint16_t hw::cal_crc_ccitt(uint8_t *ptr, uint16_t len)
{
  uint16_t crc = 0xFFFF;  //initialization value

  while (len--) {
    crc = (crc << 8) ^ CRC16_CCITT_Table[((crc >> 8) ^ *ptr++) & 0xff];
  }

  return crc;
}
void hw::rx_disable(void)
{
  write_reg(TRCTRL, 0x0000); //rx_disable
  write_reg(INTIC, 0x0001); //clr_int

  _state = IDLE;
  //this->_irq_request = 0;
  //this->_gpio1_request = 0;
}
int8_t hw::tx_data(mode_t mode, data_t *txbuf)
{
  //IF_SERIAL_DEBUG(printf("tx_data->mode=%i\r\n", mode.frame_mode));
  if (mode.frame_mode == FRAME)
  {
    return frame_tx(mode, txbuf);
  }
  else
  {
    return fifo_tx(mode, txbuf);
  }
}


int8_t hw::frame_tx(mode_t mode, data_t *txbuf)
{
  uint16_t reg_val,crc;
 
  if (txbuf->len > 252)
  {
    return -1;
  }
  crc = cal_crc_ccitt(txbuf->data, txbuf->len - 2);
      txbuf->data[txbuf->len - 2] = crc >> 8;//包后两个字节装数据包CRC校验
      txbuf->data[txbuf->len - 1] = crc;

  if (_state != TX)
  {
    _state = TX;

    write_reg(TRCTRL, 0x0100);   //tx_enable
    write_reg(FIFOSTA, 0x0100); //flush fifo
    write_fifo(FIFODATA, txbuf->data, txbuf->len); //write fifo
    write_reg(FIFOCTRL, 0x0001); //ocpy = 1
    // IF_SERIAL_DEBUG(printf("frame_tx->txbuf.data= %s len= %i\r\n", txbuf->data, txbuf->len));
  
    //while (!read_reg(0x0f) & 0x0001);
    while(1){

    IF_SERIAL_DEBUG(printf("send int:%x\r\n ",read_reg(0x0f)) );
    if(read_reg(0x0f)&1)break; 
    }
    
    write_reg(INTIC, 0x0001);
    //_irq_request = 0;
    _state = IDLE;
    //IF_SERIAL_DEBUG(printf("...............frame_tx:_state: %x\r\n",mode.ack_mode));
    if (mode.ack_mode == ENABLE)
    {
      reg_val = read_reg(INTFLAG);
      if (reg_val & 0x0002)
      {
        write_reg(FIFOCTRL, 0x0000); //ocpy = 0
        write_reg(INTIC, 0x0001);   //clr_int
        write_reg(TRCTRL, 0x0000); //send disable

        return -1;
      }
    }
    write_reg(FIFOCTRL, 0x0000); //ocpy = 0
    write_reg(INTIC, 0x0001);  //clr_int
    write_reg(TRCTRL, 0x0000);   //send disable
//IF_SERIAL_DEBUG(printf("...............frame_tx->end:_state: %x\r\n", _state));
    return 0;
  }
  return -1;
  
}

int8_t hw::fifo_tx(mode_t mode, data_t *txbuf)
{
  if (_state != TX) {
    _state = TX;

    write_reg(TRCTRL, 0x0100);   //tx_enable
    write_reg(FIFOSTA, 0x0108);  //flush fifo

    write_fifo(FIFODATA, txbuf->data, txbuf->len); //write fifo
    write_reg(FIFOCTRL, 0x0001); //ocpy = 1
    _irq_request = 0;
    while (!_irq_request); //wait for send finish
    _irq_request = 0;
    _state = IDLE;


    write_reg(FIFOCTRL, 0x0000); //ocpy = 0
    write_reg(INTIC, 0x0001); //clr_int
    write_reg(TRCTRL, 0x0000); //send disable

    return 0;
  }

  return -1;
}

int8_t hw::rx_enable(void)
{
  if (_state != TX) {
    write_reg(TRCTRL, 0x0080); //enable rx
    write_reg(FIFOSTA, 0x0200); //flush fifo

    _state = RX;
    //_irq_request = 0;
    //_gpio1_request = 0;

    return 0;
  }
  return -1;
}


int8_t hw::rx_task(data_t *_data_buf)
{
  uint16_t  crc, len, reg;


  if (_state != RX)
  {
    //_gpio1_request = 0;
    return -2;
  }

  if (rx_data(_mode, _data_buf) == 0)
  {IF_SERIAL_DEBUG(printf("in the rx_task\r\n"));
    len = _data_buf->data[0] - 2;
    crc = cal_crc_ccitt( _data_buf->data, len);
    reg =  _data_buf->data[len] * 256 + _data_buf->data[len + 1];
  IF_SERIAL_DEBUG(printf("on rx_task:%i   data:%s.......................................==\r\n",len,_data_buf->data));
    if (crc == reg)
    {
      return 0;
    }
  }

  return -1;
}
/////////////////////////////////////////////////////////////////////////////////
int8_t hw::rx_data(mode_t mode, data_t *rxbuf)
{
  uint16_t reg;


  if (_state == RX)
  {
    if (mode.frame_mode == FRAME)
    {

      reg = read_reg(FIFOCTRL);
      if (!(reg & 0xC000)) //PHR CRC check
      {
        rxbuf->len = read_reg(RXPHR0);
        rxbuf->len = ((rxbuf->len >> 8) - 3) & 0x00FF;
        read_fifo(FIFODATA, rxbuf->data, rxbuf->len);
        
        rx_disable();
        rx_enable();

        return 0;
      }
      else
      {
        rx_disable();
        rx_enable();
        return -1;

      }
    }
    else
    {
      read_fifo(FIFODATA, rxbuf->data, 1);
      rxbuf->len = rxbuf->data[0] - 1;
      read_fifo(FIFODATA, &rxbuf->data[1], rxbuf->len);
    }

    rx_disable();
    rx_enable();
    return 0;
  }
  else
  {
    return -1;
  }
}
bool hw::rx_avalible(void){
 // uint8_t ret;
 // ret=read_reg(0x0f);
  /////IF_SERIAL_DEBUG(printf("ret================!%i\r\n",ret));
  //return(ret&1);
  
return(read_reg(0x0f)&1);
  }
