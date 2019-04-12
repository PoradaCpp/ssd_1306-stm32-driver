/*
 * ssd1306.h
 *
 */

#ifndef SSD1306_H_
#define SSD1306_H_

/* CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>

#include "stm32f0xx_hal.h"
#include "fonts.h"
#include "buffer.h"

#define SWAP_UINT16_T(a, b) { uint16_t t = a; a = b; b = t; }

class SSD1306_oled;

void over(SSD1306_oled &Obj);

enum FONT {_7x10, _11x18, _16x26};

class SSD1306_oled
{
	// ==================================  Static constant private variables ========================================== //
	static const uint8_t DISPLAY_WIDTH = 128;
	static const uint8_t DISPLAY_HEIGHT = 64;
	static const uint16_t BUFFER_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT) >> 3;
	static const uint8_t PAGES_COUNT = 8;
	
	static const uint8_t SET_CONTRAST_CONTROL = 0x81;      // Address of setting of a contrast range. Demands additional data with value 1...256
	static const uint8_t RESUME_DISPLAY_RAM = 0xA4;        // Resume to RAM content display (reset state). Output follows RAM content
	static const uint8_t ENTIRE_DISPLAY_ON = 0xA5;         // Entire display ON. Output ignores RAM content 
	static const uint8_t SET_DISPLAY_NORMAL = 0xA6;        // Normal display (reset state)
	static const uint8_t SET_DISPLAY_INVERSE = 0xA7;       // Inverse display 
	static const uint8_t SET_DISPLAY_ON = 0xAF;            // Display ON in normal mode
	static const uint8_t SET_DISPLAY_OFF = 0xAE;	         // Display OFF (sleep mode)
	static const uint8_t SET_MEM_ADDRESS_MODE = 0x20;      // Set memory addressing mode. Need additional set-mode command, which listed below
	static const uint8_t _HORIS_ADDRESS_MODE = 0x00;       // This command uses after SET_MEM_ADDRESS_MODE command. Horizontal addressing mode
	static const uint8_t _VERT_ADDRESS_MODE = 0x01;        // This command uses after SET_MEM_ADDRESS_MODE command. Vertical addressing mode
	static const uint8_t _PAGE_ADDRESS_MODE = 0x02;        // This command uses after SET_MEM_ADDRESS_MODE command. Page addressing mode (reset state)
	
	// Address of setup of a column start and end address. Demands two additional data values:
  //	- column start address with range 0-127 (reset value 0)
	//  - column end address with range 0-127 (reset value 127)
	static const uint8_t SET_COLUMN_ADDRESS = 0x21;
	
	// Address of setup of a page start and end address. Demands two additional data values:
  //	- page start address with range 0-7 (reset value 0)
	//  - page end address with range 0-7 (reset value 7)
	static const uint8_t SET_PAGE_ADDRESS = 0x22;
	static const uint8_t SET_COLUMN_NORMAL_MAPPING = 0xA0; // Column address 0 is mapped to SEG0 (reset state). Nolmal displaying
	static const uint8_t SET_COLUMN_REFL_MAPPING = 0xA1;   // Column address 127 is mapped to SEG0. Horisontal reflected displaying
	static const uint8_t SET_MULTIPLEX_RATIO = 0xA8;       // Set MUX ratio to N+1 MUX. Demands additional data with value 15...63. Reset value is 63
	static const uint8_t SET_COM_NORMAL_MAPPING = 0xC0;    // Set COM output scan direction. Nolmal displaying (reset state)
	static const uint8_t SET_COM_REFL_MAPPING = 0xC8;      // Set COM output scan direction. Vertical reflected displaying (reset state)
	static const uint8_t SET_DISPLAY_OFFSET = 0xD3;        // Set vertical shift by COM. Demands additional data with value 0...63. Reset value is 0
	
	// Address of setting of the divide ratio and the oscillator frequency of the display clock.
	// Demands one 8-bit additional data value, consists of two parts:
	//  - bites 0...3. Divide ratio with range 0-15 (for reset value 0 there is prescaler 1)
	//  - bites 4...7. Oscillator frequency increases with the value and vice versa. Reset value is 8, range 0...15
	static const uint8_t SET_DISPLAY_CLOCK = 0xD5;
	static const uint8_t SET_PRECHADGE_PERIOD = 0xD9;       // Set phase 1 period (see datasheet). Demands additional data with value 1...15. Reset value is 2
	static const uint8_t CHARGE_PUMP_SETTING = 0x8D;        // Charge pump setting address. Demands additional command ENABLE_CHARGE_PUMP
	static const uint8_t ENABLE_CHARGE_PUMP = 0x14;         // Enable charge pump. Use after ENABLE_CHARGE_PUMP command
	static const uint8_t SET_COM_PIN_HW_CONF = 0xDA;        // Set COM Pins hardware configuration. Demands additional data
	static const uint8_t _SEQUENTIAL_HW_PIN_CONF = 0x00;    // Sequential COM pin configuration without right/left remapping. Use after SET_COM_PIN_HW_CONF command
	static const uint8_t _ALTERNATIVE_HW_PIN_CONF = 0x10;   // Alternative COM pin configuration without right/left remapping (reset state). Use after SET_COM_PIN_HW_CONF command
	static const uint8_t _SEQUENTIAL_HW_PIN_CONF_R = 0x20;  // Sequential COM pin configuration with right/left remapping. Use after SET_COM_PIN_HW_CONF command
	static const uint8_t _ALTERNATIVE_HW_PIN_CONF_R = 0x30; // Alternative COM pin configuration with right/left remapping. Use after SET_COM_PIN_HW_CONF command
	static const uint8_t COMMAND_TO_SEND = 0x00;            // I2C mark for device if it is command to send
	static const uint8_t COMMAND_SEND_TIMEOUT = 10;         // I2C timeout for sending
	static const uint8_t DATA_TO_SEND = 0x40;               // I2C mark for device if it is data to send
	// ------------------------------------------------------------------------------------------------------------- //
	
	// ==================================== Others private class members =========================================== //
	I2C_HandleTypeDef *m_I2C_Port;                          // STM32 I2C port
	uint16_t m_SSD1306_I2C_Address;                         // SSD1306 I2C address, bit-shifted for 1 pos left
	GPIO_TypeDef* m_GPIO_Port;                              // STM32 GPIO port for reseting OLED
	uint16_t m_GPIO_Pin;                                    // STM32 GPIO pin for reseting OLED
	uint8_t *m_pBuffer;                                     // Buffer pointer to collect information to display
	uint8_t *m_pBuffer_beg;                                 // First value of buffer pointer to transmit data
	bool m_InitState;                                       // Initialization flag
	uint8_t m_CurX;                                         // |
	uint8_t m_CurY;                                         // | Conditional position of cursor in m_pBuffer for text printing
	FONT m_DefFont;                                         // Default font for printing
	
	char_buffer cBuf;                                       // Buffer for transformation data to string-type
	// ------------------------------------------------------------------------------------------------------------- //
	
	// =================================== Private class member functions ========================================== //
	void i2c_WriteCommand(uint8_t nCommand);
	void i2c_WriteData(uint8_t *pData, uint16_t nSize);
	void reset_oled();
	void inline check_limits(uint16_t &x, uint16_t &y);
	// ------------------------------------------------------------------------------------------------------------- //
	
	public:		
	// =================================== Public class member functions ========================================== //	
	SSD1306_oled(I2C_HandleTypeDef *hi2c, uint16_t i2c_addr, GPIO_TypeDef* GPIOx, uint16_t GPIO_PIN);
	~SSD1306_oled();
	uint8_t ssd1306_Init();		
	void update_screen();
	void clear_screen();
	void clear_buffer();
	void set_pos(uint8_t pos_x_beg, uint8_t pos_y_beg, uint8_t pos_x_end = DISPLAY_WIDTH - 1, uint8_t pos_y_end = DISPLAY_HEIGHT - 1);
	void set_cursor(uint16_t x, uint16_t y);
	void draw_pixel(uint16_t x, uint16_t y);
	void draw_pixel_inverted(uint16_t x, uint16_t y);
	void draw_horisontal_line(uint16_t x, uint16_t y, uint16_t length, uint16_t thickness = 1);
	void draw_vertical_line(uint16_t x, uint16_t y, uint16_t length, uint16_t thickness = 1);
	void draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
	void draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t thickness = 1);
	void draw_fill_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
	void draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
	void draw_fill_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
	void draw_circle(uint16_t x0, uint16_t y0, uint16_t radius);
	void draw_fill_circle(uint16_t x0, uint16_t y0, uint16_t radius);
	char write_char(char ch, FontDef Font);
	char write_string(const char* str, FontDef Font);
	void set_font(FONT font);
	
	SSD1306_oled& operator << (const char ch);
	SSD1306_oled& operator << (const char *pStr);
	SSD1306_oled& operator << (int nNumber);
	SSD1306_oled& operator << (float fNumber);
	SSD1306_oled& operator << (void (*pF)(SSD1306_oled &Obj));
	void set_precision(uint8_t precision);
	
	friend void over(SSD1306_oled &Obj);
};

/* CODE END PFP */
#endif /* SSD1306_H_ */
