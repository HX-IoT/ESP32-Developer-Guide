## 前言
- ### ESP32 smartconfig配置
- ### 作者：红旭无线开发团队，QQ群：671139854
- ### 版本：Ver0.0.1，2018/08/09
- ### 代码使用步骤：
    - make clean 清除掉之前编译的残留
    - make menuconfig 修改下载程序的串口号
    - make all 编译程序
    - make flash 下载程序
    - make monitor 打开串口看log
    - 使用手机打开smartconfig app输入账号密码即可快配

- ### 程序逻辑
    - wifi进入sc代配状态
    - 手机快配ssid和密码
    - 连接快配的ssid
    - DNS获取服务器IP，esp32建立socket，使用tcp模拟http发送获取天气报文
    - 接收http报文进行json解析，串口打印天气信息

- ### app
    - ios：Esptouch或者SmartConfig
    - android：乐鑫官方有开源app
    
## 结果


