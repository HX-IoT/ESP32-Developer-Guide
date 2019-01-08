/*
* ESP32搭建tcp server、tcp client
* 作者：红旭无线开发团队，QQ群：671139854
* 版本：Ver0.0.2，2018/08/09


* 代码使用步骤：
    * 1.通过宏来选择tcp 是作为server还是client
    * 2.修改作为client连接server的ip和port,修改作为server时的port
    * 3.修改AP和STA的账号密码
    * 4.make clean 清除掉之前编译的残留
    * 5.make menuconfig 修改下载程序的串口号
    * 6.make all 编译程序
    * 7.make flash 下载程序。本人喜欢把6、7步合二为一 make all flash monitor，顺便打开串口
    * 8.作为client时，需要使用手机或者电脑使用助手工具建立server，让esp32自动连接
    * 9.作为server时，需要使用手机或者电脑使用助手工具，连接esp32的server
    
* 待完善
    * 1.作为server时，ap的密码只能为空，否则手机和电脑都连不上esp32的ap热点，原因不明。
    
* 结果
    * 1.10ms+32bytes数据强怼，wifi还是很稳定的，不丢包，不漏数据。

