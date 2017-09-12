#include "comms/fineproto.h"


void fineproto_add_sensor(uint16_t (*get_data_for)(void), Sensor sensor)
{
  _fineproto.get_data_for[sensor] = get_data_for;
}

void fineproto_init()
{
  memset((char*) &_fineproto, 0, sizeof(FineProtocol));
  _fineproto.rdi = QUEUE_SIZE-1;
  //and start DMA
  HAL_UART_Receive_DMA(&HM10_UART, (uint8_t*) &_fineproto.last_rcv, 5);
}

void fp_parse_all_messages()
{
  while(_fineproto.rdi != _fineproto.rci)
  {
    fp_parse_message(_fineproto.rcv_queue[_fineproto.rdi]);
    _fineproto.rdi++;
    _fineproto.rdi %= QUEUE_SIZE;
  }
}

void fp_parse_message(FineMessage message)
{
  if(message.header != 0xF2 || message.checksum != _fp_calculate_checksum(message))
    return;
  
  if((message.command & 0x0F) == QUERY_SENSOR)
  {
    _fineproto.to_send = _fp_create_data_message(message.command>>4);
  }
  else{
	  _fineproto.to_send.header = FINEPROTO_HEADER;
	  _fineproto.to_send.command = message.command;
	  uint16_t capabilities = 0;
	  switch(message.command)
	  {
      	  case QUERY_CAPABILITIES:
			  // iterate over func pointers, if !=0 put 1, then <<1 etc.
			  for(int i = 0; i < MAX_SENSORS; ++i)
			  {
				  if(_fineproto.get_data_for[i])
				  {
					  capabilities |= 1;
				  }
				  capabilities <<= 1;
			  }
			  _fineproto.to_send.data[0] = (capabilities >> 8) & 0xFF;
			  _fineproto.to_send.data[1] = capabilities & 0xFF;
			  break;
		  case QUERY_FWVER:
			  _fineproto.to_send.data[0] = BUILDDAY;
			  _fineproto.to_send.data[1] = BUILDMONTH;
			  break;
		  case QUERY_CONTINOUS:
			  _fineproto.continuous_timer = message.data[0]<<8 | message.data[1];
			  _fp_continuous_setup();
			  // no reply here!
			  return;
		  case QUERY_HANDSHAKE:
			  _fineproto.to_send.data[0] = message.data[0] + 0xF1;
			  _fineproto.to_send.data[1] = message.data[1] + 0xF1;
			break;
		  default:
			//unknown command, no reply.
			break;
    }
    _fineproto.to_send.checksum = _fp_calculate_checksum(_fineproto.to_send);
  }
  _fp_send_message();
}

void _fp_continuous_setup()
{
  if(_fineproto.continuous_timer == 0)
  {
    _fp_continuous_stop();
    return;
  }
  HAL_LPTIM_Counter_Start_IT(FP_CONT_MODE_LPTIM, _fineproto.continuous_timer<<8);
}

void _fp_continuous_advance()
{
  // iterate over callbacks until a configured sensor comes up 
	while(_fineproto.get_data_for[_fineproto.pri] == 0)
	{
		_fineproto.pri++;
		_fineproto.pri &= 0xF;
	}
	_fineproto.continous_to_send = _fp_create_data_message(_fineproto.pri);
	_fineproto.pri++;
	_fineproto.pri &= 0xF;
	hm10_send_message((uint8_t*)&_fineproto.continous_to_send, 5);
}

uint32_t _fp_got_message()
{
	FineMessage temp = _fineproto.last_rcv;
	if(temp.header != 0xF2 || _fp_calculate_checksum(temp) != temp.checksum)
		return 1;
	_fineproto.rcv_queue[_fineproto.rci] = _fineproto.last_rcv;
	_fineproto.rci++;
	_fineproto.rci %= QUEUE_SIZE;
	return 0;
}

FineMessage _fp_create_data_message(Sensor sensor)
{
	FineMessage fm_to_ret;
	fm_to_ret.header = 0xF1;
	fm_to_ret.command = 0x02 | (((uint8_t)sensor) << 4);
	uint16_t sensor_data = _fineproto.get_data_for[sensor]();
	fm_to_ret.data[0] = sensor_data >> 8;
	fm_to_ret.data[1] = sensor_data & 0xFF;
	fm_to_ret.checksum = _fp_calculate_checksum(fm_to_ret);
	return fm_to_ret;
}
