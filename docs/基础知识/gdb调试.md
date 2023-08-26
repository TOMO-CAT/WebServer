# gdb 调试

## 准备工具

通常在为调试而编译时，我们会加上 `-g` 选项，以在编译出的可执行文件中加入调试信息。
另外，`-Wall` 在尽量不影响程序行为的情况下打开所有 warning 也可以发现很多问题从而避免不必要的 bug。

```bash
gcc -g -Wall -o main main.c
```

> `-g` 选项的作用实在可执行文件中加入源代码的信息，比如可执行文件中第几条机器指令对应源代码第几行，但并不是把整个源文件嵌入到可执行文件中，**所以在调试时必须保证 gdb 能找到源文件**。

## 命令

```bash
# 启动
gdb [可执行程序]
# 退出
quit

# 给程序设置参数
set args 10 20
# 获取设置参数
show args

# 查看当前文件代码
# 1. 从默认位置显示
list/l
# 2. 从指定的行显示
list/l [行号]
# 3. 从指定的函数显示
list/l [函数名]

# 查看非当前文件代码
list/l [文件名:行号]
list/l [文件名:函数名]

# 设置显示的行数
show list/listsize
set list/listsize [行数]

# 设置断点
break/b [行号]
break/b [函数名]
break/b [文件名:行号]
break/b [文件名:函数名]
# 查看断点
info/i break/b
# 删除断点
delete/del/d [断点号]
# 设置断点无效
disable/dis [断点号]
# 设置断点有效
enable/ena [断点号]
# 设置条件断点(一般用在循环的位置)
break/b [行号] if [条件]

# 运行程序
# 1. 程序停在第一行
start
# 2. 遇到断点才停
run
# 3. 继续运行直到下一个断点才停
continue/c
# 4. 向下执行一行代码(不会进入函数体)
next/n

# 变量操作
# 1. 打印变量值
print/p [变量名]
# 2. 打印变量类型
ptype [变量名]

# 向下单步调试(遇到函数进入函数体)
step/s

# 跳出函数体
finish

# 自动变量操作
# 1. 自动打印指定变量的值
display num
# 2. 查看设置的自动变量
info/i display
# 3. 取消自动变量
undisplay [编号]

# 设置变量的值
set var [变量名]=[值]

# 跳出循环
until
```
