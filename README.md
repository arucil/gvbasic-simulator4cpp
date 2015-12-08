# gvbasic-simulator4cpp
GVBASIC模拟器的Win32版本, 使用C++编写.

## 特性
- 解释器核心代码平台无关
- 支持GVBASIC所有语句和函数
- 支持嵌入6502机器码, 但未完全测试
- 输入文字时支持Windows的输入法

## 语言特性
- 支持VB注释
- 标识符和关键字不区分大小写

### 新语法
- 延时语句
	```cpp
	SLEEP milliseconds
	```
    延时`milliseconds`毫秒
    
- 绘制语句
	```cpp
    PAINT addr, x,y,w,h [,mode]
	```
   绘制图片  
   `addr`&nbsp;&nbsp;&nbsp;&nbsp;图片数据地址  
   `x` `y`&nbsp;&nbsp;&nbsp;&nbsp;绘制图片左上角坐标, 可以是负数  
   `w` `h`&nbsp;&nbsp;&nbsp;&nbsp;绘制图片尺寸   
	`mode`&nbsp;&nbsp;&nbsp;&nbsp;`0`=copy&nbsp;&nbsp;`1`=or&nbsp;&nbsp;`2`=not&nbsp;&nbsp;`3`=and&nbsp;&nbsp;`4`=xor
   
 - 加载数据
 	```cpp
   LOAD addr, size, ...
   ```
	`addr`&nbsp;&nbsp;&nbsp;&nbsp;加载数据的地址  
   `size`&nbsp;&nbsp;&nbsp;&nbsp;数据字节数, 必须是常数  
   `...`&nbsp;&nbsp;&nbsp;&nbsp;数据字节

- 函数  
	- ```POINT(x, y)```   
		判断像素点是否为黑  
      `x` `y`&nbsp;&nbsp;&nbsp;&nbsp;坐标, 可以是负数
   - ```CHECKKEY(wqxkey)```  
  		判断键是否按下  
      `wqxkey`&nbsp;&nbsp;&nbsp;&nbsp;WQX键值
- 文件操作  
	- 语句  
		- ```OPEN A$ FOR BINARY AS #n```  
			打开二进制文件  
			和RANDOM模式类似, 区别是BINARY不会新建文件, 并且不能用LEN
      - ```FPUTC fnum, c$```  
			写入c$的第一字节, 用于RANDOM / BINARY
      - ```FREAD fnum, addr, size```  
			从文件读取`size`字节到地址`addr`, 用于RANDOM / BINARY
      - ```FWRITE fnum, addr, size```  
			写入地址`addr`开始的`size`字节到文件, 用于RANDOM / BINARY
      - ```FSEEK fnum, pt```  
			设置文件指针为`pt`的值, 用于RANDOM / BINARY
	- 函数  
		- ```FOPEN(fnum)```  
			判断文件是否打开
      - ```FGETC(fnum)```  
			从文件读取一字节, 用于RANDOM / BINARY
      - ```FTELL(fnum)```  
			获取文件指针