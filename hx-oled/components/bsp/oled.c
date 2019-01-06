/*
* @file         oled.c 
* @brief        ESP32操作OLED-I2C
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       红旭团队 
* @par Copyright (c):  
*               红旭无线开发团队，QQ群：671139854
*/
/* 
=============
头文件包含
=============
*/
#include "oled.h"
//#include "oledfont.h"
#include "string.h"
#include "stdlib.h"
#include "fonts.h"
/*
===========================
全局变量定义
=========================== 
*/
//OLED缓存128*64bit
static uint8_t g_oled_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
//OLED实时信息
static SSD1306_t oled;
//OLED是否正在显示，1显示，0等待
static bool is_show_str =0;
/*
===========================
函数定义
=========================== 
*/

/** 
 * oled_i2c 初始化
 * @param[in]   NULL
 * @retval      
 *              NULL                              
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 *               Ver0.0.2:
                     hx-zsj, 2018/08/07, 统一编程风格\n 
 */
void i2c_init(void)
{
    //注释参考sht30之i2c教程
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_OLED_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_OLED_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    i2c_param_config(I2C_OLED_MASTER_NUM, &conf);
    i2c_driver_install(I2C_OLED_MASTER_NUM, conf.mode,0, 0, 0);
}

/** 
 * 向oled写命令
 * @param[in]   command
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 *               Ver0.0.2:
                     hx-zsj, 2018/08/07, 统一编程风格\n 
 */

int oled_write_cmd(uint8_t command)
{
    //注释参考sht30之i2c教程
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, OLED_WRITE_ADDR |WRITE_BIT , ACK_CHECK_EN); 
    ret = i2c_master_write_byte(cmd, WRITE_CMD, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd,command, ACK_CHECK_EN);
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_OLED_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    return ret;
}

/** 
 * 向oled写数据
 * @param[in]   data
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
int oled_write_data(uint8_t data)
{
    //注释参考sht30之i2c教程
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, OLED_WRITE_ADDR | WRITE_BIT, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd, WRITE_DATA, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_OLED_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    return ret;
}
/** 
 * 向oled写长数据
 * @param[in]   data   要写入的数据
 * @param[in]   len     数据长度
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
int oled_write_long_data(uint8_t *data,uint16_t len)
{
    //注释参考sht30之i2c教程
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, OLED_WRITE_ADDR | WRITE_BIT, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd, WRITE_DATA, ACK_CHECK_EN);
    ret = i2c_master_write(cmd, data, len,ACK_CHECK_EN);
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_OLED_MASTER_NUM, cmd, 10000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    return ret;    
}

/** 
 * 初始化 oled
 * @param[in]   NULL
 * @retval      
 *              NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
void oled_init(void)
{
    //i2c初始化
    i2c_init();
    //oled配置
    oled_write_cmd(TURN_OFF_CMD);
    oled_write_cmd(0xAE);//关显示
    oled_write_cmd(0X20);//低列地址
    oled_write_cmd(0X10);//高列地址
    oled_write_cmd(0XB0);//
    oled_write_cmd(0XC8);
    oled_write_cmd(0X00);
    oled_write_cmd(0X10);
     //设置行显示的开始地址(0-63)  
    //40-47: (01xxxxx)  
    oled_write_cmd(0X40);
     //设置对比度  
    oled_write_cmd(0X81);
    oled_write_cmd(0XFF);//这个值越大，屏幕越亮(和上条指令一起使用)(0x00-0xff) 

    oled_write_cmd(0XA1);//0xA1: 左右反置，  0xA0: 正常显示（默认0xA0）
   //0xA6: 表示正常显示（在面板上1表示点亮，0表示不亮）  
    //0xA7: 表示逆显示（在面板上0表示点亮，1表示不亮）
    oled_write_cmd(0XA6); 

    oled_write_cmd(0XA8);//设置多路复用率（1-64） 
    oled_write_cmd(0X3F);//（0x01-0x3f）(默认为3f)
    oled_write_cmd(0XA4);
    //设置显示抵消移位映射内存计数器  
    oled_write_cmd(0XD3);
    oled_write_cmd(0X00);
    //设置显示时钟分频因子/振荡器频率 
    oled_write_cmd(0XD5);
    //低4位定义显示时钟(屏幕的刷新时间)（默认：0000）分频因子= [3:0]+1  
    //高4位定义振荡器频率（默认：1000） 
    oled_write_cmd(0XF0);
    //时钟预充电周期  
    oled_write_cmd(0XD9);
    oled_write_cmd(0X22);
    //设置COM硬件应脚配置  
    oled_write_cmd(0XDA);
    oled_write_cmd(0X12);
    oled_write_cmd(0XDB);
    oled_write_cmd(0X20);
    //电荷泵设置（初始化时必须打开，否则看不到显示）
    oled_write_cmd(0X8D);
    oled_write_cmd(0X14);
    //开显示
    oled_write_cmd(0XAF);
    //清屏
    oled_claer();
}

/** 
 * 将显存内容刷新到oled显示区
 * @param[in]   NULL
 * @retval      
 *              NULL                           
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
void oled_update_screen(void)
{
    uint8_t line_index;
    for(line_index=0    ;   line_index<8   ;  line_index++)
    {
        oled_write_cmd(0xb0+line_index);
        oled_write_cmd(0x00);
        oled_write_cmd(0x10);
        
        oled_write_long_data(&g_oled_buffer[SSD1306_WIDTH * line_index],SSD1306_WIDTH);
    }
}

/** 
 * 清屏
 * @param[in]   NULL
 * @retval      
 *              NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
void oled_claer(void)
{
    //清0缓存
    memset(g_oled_buffer,SSD1306_COLOR_BLACK,sizeof(g_oled_buffer));
    oled_update_screen();
}
/** 
 * 填屏
 * @param[in]   NULL
 * @retval      
 *              NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
void oled_all_on(void)
{
    //置ff缓存
    memset(g_oled_buffer,0xff,sizeof(g_oled_buffer));
    oled_update_screen();
}
/** 
 * 移动坐标
 * @param[in]   x   显示区坐标 x
 * @param[in]   y   显示去坐标 y
 * @retval      
 *              其它                         
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
void oled_gotoXY(uint16_t x, uint16_t y) 
{
	oled.CurrentX = x;
	oled.CurrentY = y;
}
/** 
 * 向显存写入
 * @param[in]   x   坐标
 * @param[in]   y   坐标
 * @param[in]   color   色值0/1
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
void oled_drawpixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color) 
{
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) 
    {
		return;
	}
	if (color == SSD1306_COLOR_WHITE) 
	{
		g_oled_buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	} 
    else
    {
		g_oled_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}
/** 
 * 在x，y位置显示字符
 * @param[in]   x    显示坐标x 
 * @param[in]   y    显示坐标y 
 * @param[in]   ch   要显示的字符
 * @param[in]   font 显示的字形
 * @param[in]   color 颜色  1显示 0不显示
 * @retval      
 *              其它                        
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
char oled_show_char(uint16_t x,uint16_t y,char ch, FontDef_t* Font, SSD1306_COLOR_t color) 
{
	uint32_t i, b, j;
	if ( SSD1306_WIDTH <= (oled.CurrentX + Font->FontWidth) || SSD1306_HEIGHT <= (oled.CurrentY + Font->FontHeight) ) 
    {
		return 0;
	}
	if(0 == is_show_str)
    {
        oled_gotoXY(x,y);
    }

	for (i = 0; i < Font->FontHeight; i++) 
    {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (j = 0; j < Font->FontWidth; j++)
        {
			if ((b << j) & 0x8000) 
            {
				oled_drawpixel(oled.CurrentX + j, (oled.CurrentY + i), (SSD1306_COLOR_t) color);
			} 
            else 
            {
				oled_drawpixel(oled.CurrentX + j, (oled.CurrentY + i), (SSD1306_COLOR_t)!color);
			}
		}
	}
	oled.CurrentX += Font->FontWidth;
	if(0 == is_show_str)
    {
       oled_update_screen(); 
    }
	return ch;
}
/** 
 * 在x，y位置显示字符串 
 * @param[in]   x    显示坐标x 
 * @param[in]   y    显示坐标y 
 * @param[in]   str   要显示的字符串
 * @param[in]   font 显示的字形
 * @param[in]   color 颜色  1显示 0不显示
 * @retval      
 *              其它                        
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
char oled_show_str(uint16_t x,uint16_t y, char* str, FontDef_t* Font, SSD1306_COLOR_t color) 
{
    is_show_str=1;
    oled_gotoXY(x,y);
	while (*str) 
    {
		if (oled_show_char(x,y,*str, Font, color) != *str) 
        {
            is_show_str=0;
			return *str;
		}
		str++;
	}
    is_show_str=0;
    oled_update_screen();
	return *str;
}
