输入文字时可以使用windows的输入法
支持VB注释

sleep milliseconds

paint addr, x,y,w,h [,mode]
x,y可以是负数
mode	0:copy 1:or 2:not 3:and 4:xor

load addr, int, ...
int必须是常数

point(x,y)
x,y可以是负数

checkkey(wqxkey)

/////////////////////////////

open a$ for binary as #n
和random类似，区别是binary不会新建文件，并且不能用len

用于所有文件:
fopen(fnum)
文件是否打开

用于random或binary:

fgetc(fnum)
读取一字节(num)

fputc fnum, c$
写入c$的第一字节

fread fnum, addr, size

fwrite fnum, addr, size

ftell(fnum)

fseek fnum, pt