
#include "ssd1306.h"

SSD1306_oled::SSD1306_oled(I2C_HandleTypeDef *hi2c, uint16_t i2c_addr, GPIO_TypeDef* GPIOx, uint16_t GPIO_PIN):
		m_I2C_Port(hi2c),	m_SSD1306_I2C_Address(i2c_addr << 1), m_GPIO_Port(GPIOx), m_GPIO_Pin(GPIO_PIN), m_InitState(0)
	{
		m_pBuffer = new uint8_t[BUFFER_SIZE];
		memset(m_pBuffer, 0, BUFFER_SIZE);
		ssd1306_Init();
		m_DefFont = _7x10;
	}
	
	SSD1306_oled:: ~SSD1306_oled()
	{
		if(m_pBuffer)
			delete [] m_pBuffer;
	}

	void SSD1306_oled::i2c_WriteCommand(uint8_t nCommand)
	{
		HAL_I2C_Mem_Write(m_I2C_Port, m_SSD1306_I2C_Address, COMMAND_TO_SEND, sizeof(uint8_t), &nCommand, 1, COMMAND_SEND_TIMEOUT);
	}
	
	void SSD1306_oled::i2c_WriteData(uint8_t *pData, uint16_t nSize)
	{
		HAL_I2C_Mem_Write(m_I2C_Port, m_SSD1306_I2C_Address, DATA_TO_SEND, sizeof(uint8_t), pData, nSize, 10 * COMMAND_SEND_TIMEOUT);
	}
	
	void SSD1306_oled::reset_oled()
	{
		HAL_Delay(100);
		HAL_GPIO_WritePin(m_GPIO_Port, m_GPIO_Pin, GPIO_PIN_RESET);
		HAL_Delay(10);
		HAL_GPIO_WritePin(m_GPIO_Port, m_GPIO_Pin, GPIO_PIN_SET);
		HAL_Delay(10);
	}
	
	void inline SSD1306_oled::check_limits(uint16_t &x, uint16_t &y)
	{
		if(x > DISPLAY_WIDTH - 1)
			x = DISPLAY_WIDTH - 1;
	
		if(y > DISPLAY_HEIGHT - 1)
			y = DISPLAY_HEIGHT - 1;
	}
	
uint8_t SSD1306_oled::ssd1306_Init()
{
	HAL_Delay(500);
	if(HAL_I2C_IsDeviceReady(m_I2C_Port, m_SSD1306_I2C_Address, 5, 1000) != HAL_OK)  // Check if OLED connected to I2C
	{
		m_InitState = 0;
		return 0;                                                                      // Return false
	}
	HAL_Delay(100);
	reset_oled();
	HAL_Delay(100);
	
	i2c_WriteCommand(SET_MULTIPLEX_RATIO);
	i2c_WriteCommand(0x3F);                                                          // Leave reset value, 63 + 1 = 64, for displaying all screen
	i2c_WriteCommand(SET_DISPLAY_OFFSET);
	i2c_WriteCommand(0x00);                                                          // Leave reset value, 0, without shifting
	i2c_WriteCommand(0x40);                                                          // Set display start line 0
	i2c_WriteCommand(SET_COLUMN_REFL_MAPPING);
	i2c_WriteCommand(SET_COM_REFL_MAPPING);
	i2c_WriteCommand(SET_COM_PIN_HW_CONF);                                           // Set COM Pins hardware configuration,
	i2c_WriteCommand(_ALTERNATIVE_HW_PIN_CONF);                                      // leave reset value
	i2c_WriteCommand(SET_CONTRAST_CONTROL);
	i2c_WriteCommand(128);                                                           // Middle value of contrast
	i2c_WriteCommand(RESUME_DISPLAY_RAM);                                            // Disable entire display on
	i2c_WriteCommand(SET_DISPLAY_NORMAL);
	i2c_WriteCommand(SET_DISPLAY_CLOCK);
	i2c_WriteCommand(240);                                                           // For frequency value 15 << 4 | prescaler 0 (+1 in fact)
	i2c_WriteCommand(CHARGE_PUMP_SETTING);
	i2c_WriteCommand(ENABLE_CHARGE_PUMP);
	i2c_WriteCommand(SET_MEM_ADDRESS_MODE);
	i2c_WriteCommand(_HORIS_ADDRESS_MODE);
	i2c_WriteCommand(SET_DISPLAY_ON);	
	
	HAL_Delay(20);
	clear_screen();		
	update_screen();
	m_InitState = 1;
	
	return 1;                                                                        // Return OK
}

void SSD1306_oled::update_screen()
{
	set_pos(0, 0, DISPLAY_WIDTH - 1, PAGES_COUNT - 1);
	i2c_WriteData(m_pBuffer, BUFFER_SIZE);
}

void SSD1306_oled::clear_screen()
{
	memset(m_pBuffer, 0, BUFFER_SIZE);
	update_screen();
}

void SSD1306_oled::clear_buffer()
{
	memset(m_pBuffer, 0, BUFFER_SIZE);
}

void SSD1306_oled::set_pos(uint8_t pos_x_beg, uint8_t pos_y_beg, uint8_t pos_x_end, uint8_t pos_y_end)
{
	if(pos_x_beg > DISPLAY_WIDTH - 1)
		pos_x_beg = DISPLAY_WIDTH - 1;
	if(pos_x_end > DISPLAY_WIDTH - 1)
		pos_x_end = DISPLAY_WIDTH - 1;
	
	if(pos_y_beg > PAGES_COUNT - 1)
		pos_y_beg = PAGES_COUNT - 1;
	if(pos_y_end > PAGES_COUNT - 1)
		pos_y_end = PAGES_COUNT - 1;	
		
	i2c_WriteCommand(SET_COLUMN_ADDRESS);
	i2c_WriteCommand(pos_x_beg);                                                   // |
	i2c_WriteCommand(pos_x_end);                                                   // | Columns 0...127
	i2c_WriteCommand(SET_PAGE_ADDRESS);
	i2c_WriteCommand(pos_y_beg);                                                   // |
	i2c_WriteCommand(pos_y_end);                                                   // | Pages 0...7
	HAL_Delay(5);	
}

void SSD1306_oled::set_cursor(uint16_t x, uint16_t y)
{
	check_limits(x, y);
	m_CurX = x;
	m_CurY = y;
}

void SSD1306_oled::draw_pixel(uint16_t x, uint16_t y)
{
	check_limits(x, y);
	m_pBuffer[x + (y / 8) * DISPLAY_WIDTH] |= 1 << (y % 8);
}

void SSD1306_oled::draw_pixel_inverted(uint16_t x, uint16_t y)
{
	check_limits(x, y);
	m_pBuffer[x + (y / 8) * DISPLAY_WIDTH] &= ~(1 << (y % 8));
}

void SSD1306_oled::draw_horisontal_line(uint16_t x, uint16_t y, uint16_t length, uint16_t thickness)
{
	check_limits(x, y);
	if ( (x + length) > DISPLAY_WIDTH - 1)
    length = (DISPLAY_WIDTH - x);
	if ( (y + thickness) > DISPLAY_HEIGHT - 1)
    thickness = (DISPLAY_HEIGHT - y);
	
	uint8_t *bufferPtr;
	uint8_t drawBit;
	
	for(uint16_t i = 0, tempLenght = length; i < thickness; ++i, ++y, length = tempLenght)
	{
		bufferPtr = m_pBuffer;		
		bufferPtr += (y >> 3) * DISPLAY_WIDTH;
		bufferPtr += x;
		drawBit = 1 << (y & 7);
	
		while (length--)
			*bufferPtr++ |= drawBit;
	}
}

void SSD1306_oled::draw_vertical_line(uint16_t x, uint16_t y, uint16_t length, uint16_t thickness)
{
	check_limits(x, y);
	if ( (y + length) > DISPLAY_HEIGHT - 1)
    length = (DISPLAY_HEIGHT - y);
	if ( (x + thickness) > DISPLAY_WIDTH - 1)
    thickness = (DISPLAY_WIDTH - x);
	
	uint8_t *bufferPtr = m_pBuffer, *tmpBufferPtr;
	bufferPtr += (y >> 3) * DISPLAY_WIDTH;
  bufferPtr += x;
	tmpBufferPtr = bufferPtr;
	
	uint8_t drawByte = (0xFF << (y % 8)) & 0xFF;
	for(uint16_t i = 0; i < thickness; ++i)
		*bufferPtr++ |= drawByte;	
	bufferPtr = tmpBufferPtr += DISPLAY_WIDTH;
	length -= (0xFF == drawByte ? 8 : (8 - (y % 8)));
	
	for(drawByte = 0xFF ; length >= 8; length -= 8, bufferPtr += DISPLAY_WIDTH)
	{
		tmpBufferPtr = bufferPtr;
		for(uint16_t i = 0; i < thickness; ++i)
			*bufferPtr++ |= drawByte;	
		bufferPtr = tmpBufferPtr;
	}
	if(length)
	{
		drawByte = 0xFF >> (8 - length);
		for(uint16_t i = 0; i < thickness; ++i)
			*bufferPtr++ |= drawByte;
	}
}

void SSD1306_oled::draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	check_limits(x0, y0);
	check_limits(x1, y1);
	
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if(steep)
	{
		SWAP_UINT16_T(x0, y0);
		SWAP_UINT16_T(x1, y1);
	}
	if(x0 > x1)
	{
		SWAP_UINT16_T(x0, x1);
		SWAP_UINT16_T(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t yStep;

	yStep = y0 < y1 ? 1 : -1;

	for( ; x0 <= x1; ++x0)
	{
		if(steep)
			draw_pixel(y0, x0);
		else
			draw_pixel(x0, y0);

		err -= dy;
		if (err < 0)
		{
			y0 += yStep;
			err += dx;
		}
	}
}

void SSD1306_oled::draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t thickness)
{
	draw_horisontal_line(x, y, width, thickness);
	draw_horisontal_line(x, y + height, width, thickness);
	draw_vertical_line(x, y, height, thickness);
	draw_vertical_line(x + width, y, height + thickness, thickness);
}

void SSD1306_oled::draw_fill_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	draw_horisontal_line(x, y, width, height);
}

void SSD1306_oled::draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
	draw_line(x1, y1, x2, y2);
	draw_line(x2, y2, x3, y3);
	draw_line(x3, y3, x1, y1);
}

void SSD1306_oled::draw_fill_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
	check_limits(x1, y1);
	check_limits(x2, y2);
	check_limits(x3, y3);
	
	int16_t dX = 0, dY = 0, x = 0, y = 0, xInc1 = 0, xInc2 = 0,	yInc1 = 0, yInc2 = 0,
					den = 0, num = 0, numAdd = 0, numPixels = 0, curPixel = 0;

	dX = abs(x2 - x1);
	dY = abs(y2 - y1);
	x = x1;
	y = y1;

	xInc1 = x2 >= x1 ? 1 : -1;
	xInc2 = x2 >= x1 ? 1 : -1;

	yInc1 = y2 >= y1 ? 1 : -1;
	yInc2 = y2 >= y1 ? 1 : -1;

	if (dX >= dY)
	{
		xInc1 = 0;
		yInc2 = 0;
		den = dX;
		num = dX / 2;
		numAdd = dY;
		numPixels = dX;
	}
	else
	{
		xInc2 = 0;
		yInc1 = 0;
		den = dY;
		num = dY / 2;
		numAdd = dX;
		numPixels = dY;
	}

	for (curPixel = 0; curPixel <= numPixels; curPixel++)
	{
		draw_line(x, y, x3, y3);

		num += numAdd;
		if (num >= den)
		{
			num -= den;
			x += xInc1;
			y += yInc1;
		}
		x += xInc2;
		y += yInc2;
	}
}

void SSD1306_oled::draw_circle(uint16_t x0, uint16_t y0, uint16_t radius)
{
	check_limits(x0, y0);
	
	int16_t x = 0, y = radius;
	int16_t dp = 1 - radius;
	do
	{
		if(dp < 0)
			dp = dp + 2 * (++x) + 3;
		else
			dp = dp + 2 * (++x) - 2 * (--y) + 5;

		draw_pixel(x0 + x, y0 + y);     // For the 8 octants
		draw_pixel(x0 - x, y0 + y);
		draw_pixel(x0 + x, y0 - y);
		draw_pixel(x0 - x, y0 - y);
		draw_pixel(x0 + y, y0 + x);
		draw_pixel(x0 - y, y0 + x);
		draw_pixel(x0 + y, y0 - x);
		draw_pixel(x0 - y, y0 - x);

	}while (x < y);

	draw_pixel(x0 + radius, y0);
	draw_pixel(x0, y0 + radius);
	draw_pixel(x0 - radius, y0);
	draw_pixel(x0, y0 - radius);
}

void SSD1306_oled::draw_fill_circle(uint16_t x0, uint16_t y0, uint16_t radius)
{
	check_limits(x0, y0);
	int16_t x = 0, y = radius;
  int16_t dp = 1 - radius;
  do
  {
	  if(dp < 0)
		  dp = dp + 2 * (++x) + 3;
	  else
		  dp = dp + 2 * (++x) - 2 * (--y) + 5;

    draw_horisontal_line(x0 - x, y0 - y, 2*x);
    draw_horisontal_line(x0 - x, y0 + y, 2*x);
    draw_horisontal_line(x0 - y, y0 - x, 2*y);
    draw_horisontal_line(x0 - y, y0 + x, 2*y);

  } while (x < y);
	
  draw_horisontal_line(x0 - radius, y0, 2 * radius);
}

char SSD1306_oled::write_char(char ch, FontDef Font)
{
	uint32_t b;

	// Check remaining space on current line
	if (DISPLAY_WIDTH <= (m_CurX + Font.m_Width) ||	DISPLAY_HEIGHT <= (m_CurY + Font.m_Height))
		return 0;  		                                       // Not enough space on current line
	                                           
	for (uint8_t i = 0; i < Font.m_Height; i++)            // Use the font to write
	{
		b = Font.m_pData[(ch - 32) * Font.m_Height + i];
		for (uint8_t j = 0; j < Font.m_Width; j++)
		{
			if ((b << j) & 0x8000)
				draw_pixel(m_CurX + j, m_CurY + i);
			else
				draw_pixel_inverted(m_CurX + j, m_CurY + i);
		}
	}	
	m_CurX += Font.m_Width;                                // The current space is now taken
	
	return ch;                                             // Return written char for validation
}

char SSD1306_oled::write_string(const char *str, FontDef Font)
{	
	while (*str)                                           // Write until null-byte
	{
		if (write_char(*str, Font) != *str)			
			return *str;                                       // Char could not be written		
		str++;                                               // Next char
	}
	
	return *str;                                           // Everything ok
}

void SSD1306_oled::set_font(FONT font)
{
	if(font < _16x26)
		m_DefFont = font;
}

SSD1306_oled& SSD1306_oled::operator << (const char ch)
{
	cBuf << ch;
	
  return *this;
}

SSD1306_oled& SSD1306_oled::operator << (const char *pStr)
{
	cBuf << pStr;
	
  return *this;
}

SSD1306_oled& SSD1306_oled::operator << (int nNumber)
{
	cBuf << nNumber;
	
  return *this;
}

SSD1306_oled& SSD1306_oled::operator << (float fNumber)
{
	cBuf << fNumber;
	
  return *this;
}

SSD1306_oled& SSD1306_oled::operator << (void (*pF)(SSD1306_oled &Obj))
{
	pF(*this);
	
  return *this;
}

void SSD1306_oled::set_precision(uint8_t precision)
{
	cBuf.set_prec(precision);
}

void over(SSD1306_oled &Obj)
{
	switch(Obj.m_DefFont)
	{
		case _7x10:
			Obj.write_string(Obj.cBuf, Font_7x10);
		break;
		case _11x18:
			Obj.write_string(Obj.cBuf, Font_11x18);
		break;
		case _16x26:
			Obj.write_string(Obj.cBuf, Font_16x26);
		break;
			
	}
	Obj.cBuf.clear_buffer();
}
