## 前言
- ESP32搭建websocket server
- 作者：红旭无线开发团队，QQ群：671139854
- 版本：Ver0.0.1，2018/11/22


- 代码使用步骤：
    1. 修改STA的账号密码
    2. make clean 清除掉之前编译的残留
    3. make menuconfig 修改下载程序的串口号
    4. make all flash monitor，编译下载，顺便打开串口
    5. 监听内网IP下的9998端口
    6. 使用[在线websocket工具](http://www.blue-zero.com/WebSocket/),连接后即可数据交互
    
- 待完善
    - 无
    
- 结果
    - 1.web控制LED灯

More Infos here:
http://www.barth-dev.de/websockets-on-the-esp32/
