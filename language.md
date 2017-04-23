模拟器增加了一些新语法，用以补充模拟器所欠缺的真实GVBASIC环境的特性。

## 语言特性
- 标识符和关键字不区分大小写

### 新语法
- 延时语句
	```
	SLEEP ticks
	```
    延时若干个tick，具体请查看配置文件config.ini
    
- 绘制语句
	```
    PAINT addr, x,y,w,h [,mode]
	```
   绘制图片。和Lava的WriteBlock类似  
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
      - ```FPUTC fnum, m_c$```  
			写入c$的第一字节, 用于BINARY模式
      - ```FREAD fnum, addr, size```  
			从文件读取`size`字节到地址`addr`, 用于BINARY模式
      - ```FWRITE fnum, addr, size```  
			写入地址`addr`开始的`size`字节到文件, 用于BINARY模式
      - ```FSEEK fnum, pt```  
			设置文件指针为`pt`的值, 用于BINARY模式
	- 函数  
		- ```FOPEN(fnum)```  
			判断文件是否打开
      - ```FGETC(fnum)```  
			从文件读取一字节, 用于BINARY模式
      - ```FTELL(fnum)```  
			获取文件指针
