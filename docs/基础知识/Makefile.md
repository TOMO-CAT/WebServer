# Makefile

## 简介

`Makefile`文件描述了整个`C++`工程的编译和链接规则，用于产生最终的可执行文件。工程中编写`Makefile`的好处是可以通过`make`命令实现一键“自动化编译”。

## 优势

* 统一管理整个工程的编译
* 根据目标文件和依赖文件的更新时间决定是否重新编译，而不用每次编译整个工程
* 保留编译命令，后期在Makefile中修改即可，无须重写

## 工作原理

### 1. 生成目标文件

首先检查规则中的依赖文件是否存在，如果不存在则寻找是否有规则用于生成该依赖文件。

### 2. 更新目标文件

检查目标依赖文件是否有更新，如果目标文件的更新时间晚于依赖文件的更新时间，那么重新编译该依赖文件并生成新的目标文件。

## 文件格式

Makefile文件由一系列规则（rules）构成。每条规则的形式如下：

* target：目标（必须）
* prerequisites：前置条件（可选）
* commands：命令（可选）

```makefile
<target> : <prerequisites> 
[tab]  <commands>
```

### 1. 目标target

一个目标构成一条规则，目标通常是文件名，表明Make命令需要构建的对象。目标可以是一个文件，也可以是多个文件，中间用空格分隔开。

除了文件名，目标也可以是某个操作的名字，这被称为“伪目标”（phony target），例如：

```makefile
clean:
    rm *.o
```

上面makefile的目标是clean，它不是文件名而是一个操作的名字，属于“伪目标”，作用是删除对象文件。**但是如果当前目录中正好有一个文件的名字叫做clean那么这个命令就不会执行**。因为make命令发现clean文件已经存在就会认为没有必要重新构建。为了避免这种情况，我们可以明确声明clean是伪目标：

```makefile
.PHONY: clean
clean:
    rm *.o temp
```

声明成“伪目标”后，make命令就不会去检查是否存在一个clean的文件，而是每次都执行相同的命令。

另外，如果make命令运行时没有指定目标，默认会执行Makefile的第一个目标：

```bash
# 执行makefile中第一个目标
$make
```

### 2. 前置条件prerequisites

前置条件通常是一组文件名，彼此用空格分隔。它指定了"目标"是否重新构建的判断标准：只要有一个前置文件不存在，或者有过更新（前置文件的last-modification时间戳比目标的时间戳新），"目标"就需要重新构建。

```makefile
result.txt: source.txt
    cp source.txt result.txt
```

上面代码中，构建result.txt 的前置条件是 source.txt 。如果当前目录中，source.txt 已经存在，那么`make result.txt`可以正常运行，否则必须再写一条规则，来生成 source.txt 。

### 3. 命令commands

命令表示如何更新目标文件，由一行或多行的shell命令组成。每行命令之前必须有一个Tab键，如果想用其他键，可以使用内置变量`.RECIPEPREFIX`声明：

```makefile
# 使用 > 替代tab键
.RECIPEPREFIX = >
all:
> echo Hello, world
```

需要注意的是每行命令在一个单独的shell中执行，这些shell之间没有继承关系：

```makefile
# 执行 make var-lost后取不到foo的值， 因为两行命令子啊不同的进程中执行
var-lost:
    export foo=bar
    echo "foo=[$$foo]"
    
# 解决方法一: 将两行命令写在一行, 中间用分号分隔
var-kept:
    export foo=bar; echo "foo=[$$foo]"
    
# 解决方法二: 在换行符前加反斜杠转义
var-kept:
    export foo=bar; \
    echo "foo=[$$foo]"

# 解决方法三: 加上.ONESHELL:命令
.ONESHELL:
var-kept:
    export foo=bar; 
    echo "foo=[$$foo]"
```

## 语法

### 1. 注释

井号（#）在Makefile中表示注释。

### 2. 回声echoing

正常情况下，make会打印每条命令，然后再执行，这就叫做回声（echoing）。

```makefile
test:
    echo "test"
```

执行上面的规则，会得到如下结果：

```bash
$make
echo "test"
test
```

在命令前加上符号`@`，就可以关闭回声：

```makefile
test:
    @echo "test"
```

得到的结果如下：

```bash
$make
test
```

由于在构建过程中需要直到当前正在执行哪条命令，因此通常只在注释和纯显示的echo命令前面加上`@`关闭回声。

### 3. 通配符

通配符wildcard用来指定一组符合条件的文件名。Makefile 的通配符与 Bash 一致，主要有星号（*）、问号（？）和 [...] 。例如 `*.o` 表示所有后缀名为o的文件。

```makefile
clean:
    rm -f *.o
```

### 4. 模式匹配

Make命令允许对文件名，进行类似正则运算的匹配，主要用到的匹配符是`%`。比如，假定当前目录下有`f1.c`和`f2.c`两个源码文件，需要将它们编译为对应的对象文件：

```makefile
%.o: %.c

# 等价写法
f1.o: f1.c
f2.o: f2.c
```

通过使用模式匹配，我们可以将大量同类型的文件，只用一条规则就完成构建。

### 5 .变量和赋值运算符

Makefile 允许使用等号自定义变量：

```makefile
txt = Hello World
test:
    @echo $(txt)
```

上面代码中，变量 txt 等于 Hello World。调用时，变量需要放在 `$( )` 之中。

调用Shell变量，需要在美元符号前，再加一个美元符号，这是因为Make命令会对美元符号转义：

```makefile
test:
    @echo $$HOME
```

有时变量的值可能指向另一个变量：、

```makefile
v1 = $(v2)
```

上面代码中，变量 v1 的值是另一个变量 v2。这时会产生一个问题，v1 的值到底在定义时扩展（静态扩展），还是在运行时扩展（动态扩展）？如果 v2 的值是动态的，这两种扩展方式的结果可能会差异很大。

为了解决类似问题，Makefile一共提供了四个赋值运算符 （=、:=、？=、+=）：

```makefile
# 在执行时扩展，允许递归扩展
VARIABLE = value

# 在定义时扩展
VARIABLE := value

# 只有在该变量为空时才设置值
VARIABLE ?= value

# 将值追加到变量的尾端
VARIABLE += value
```

### 6. 内置变量

Make命令提供一系列内置变量，比如，$(CC) 指向当前使用的编译器，$(MAKE) 指向当前使用的Make工具：

```makefile
output:
    $(CC) -o output input.c
```

这主要是为了跨平台的兼容性，详细的内置变量清单见[手册](https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html)。

### 7. 自动变量

Make命令还提供一些自动变量，它们的值与当前规则有关。主要有以下几个。

#### 7.1 $@

`$@`指代当前目标，就是Make命令当前构建的那个目标。比如，`make foo`的 `$@` 就指代`foo`：

```makefile
a.txt: 
    touch $@

# 等价于
a.txt:
    touch a.txt
```

#### 7.2 $<

`$<`表示第一个前置条件，例如：

```makefile
a.txt: b.txt c.txt
    cp $< $@

# 等价于
a.txt: b.txt c.txt
    cp b.txt a.txt 
```

#### 7.3 $?

$? 指代比目标更新的所有前置条件，之间以空格分隔。

#### 7.4 $^

`$^`指代所有前置条件，之间以空格分隔。

#### 7.5 $*

`$*`指代匹配符`%`匹配的部分，比如`%`匹配`fi.txt`，那么`$*`就表示`fi`。

#### 7.6 `$(@D)`和`$(@F)`

`$(@D)` 和 `$(@F)` 分别指向 `$@` 的目录名和文件名。例如`$@`是`src/input.c`，那么`$(@D)`值为`src`而`$(@F)`值为`input`。

#### 7.7 `$(<D)`和`$(<F)`

`$(<D)`和`$(<F)`分别指向`$<`的目录名和文件名。

### 8.判断

Makefile使用 Bash 语法，完成判断：

```makefile
# 判断当前编译器是否 gcc, 然后指向不同的库文件
ifeq ($(CC),gcc)
  libs=$(libs_for_gcc)
else
  libs=$(normal_libs)
endif
```

### 9. 循环

Makefile使用 Bash 语法，完成循环：

```makefile
LIST = one two three
all:
    for i in $(LIST); do \
        echo $$i; \
    done

# 等同于

all:
    for i in one two three; do \
        echo $i; \
    done
```

输出结果为：

```bash
one
two
three
```

## 内置函数

Makefile提供了许多[内置函数](https://www.gnu.org/software/make/manual/html_node/Functions.html)，可供调用。下面是几个常用的内置函数。

### 1. shell函数

shell 函数用来执行 shell 命令：

```makefile
srcfiles := $(shell echo src/{00..99}.txt)
```

### 2. wildcard函数

wildcard 函数用来在 Makefile 中替换 Bash 的通配符：

```makefile
# 用法
$(wildcard PATTERN...)

# 例子：查找当前目录和./src/main目录下的所有.cpp文件
# 执行完之后src的值为多个.cpp文件
# 需要注意的是在./src/main下面的cpp会带有相对路径前缀
srcfiles = $(wildcard *.cpp ./src/main/*.cpp)
```

### 3. subst函数

subst 函数用来文本替换：

```makefile
# 用法
$(subst from,to,text)

# 例子: 将字符串"feet on the street"替换成"fEEt on the strEEt"
$(subst ee,EE,feet on the street)
```

### 4. patsubst

patsubst 函数用于模式匹配的替换：

```makefile
# 用法
$(patsubst pattern,replacement,text)

# 例子: 把所有的后缀为.c的文件名替换为.o文件名
obj = $(patsubst %.c, %.o, $(src))

# 例子: 将文件名"x.c.c bar.c"，替换成"x.c.o bar.o"
$(patsubst %.c,%.o,x.c.c bar.c)
```

替换后缀名函数的写法是：变量名 + 冒号 + 后缀名替换规则。它实际上patsubst函数的一种简写形式：

```makefile
# 将变量OUTPUT中的后缀名 .js 全部替换成 .min.js
min: $(OUTPUT:.js=.min.js)
```

### 5. notdir

```bash
# 去除路径，保留文件名
file = $(notdir $(src))
```

### 6. info

``` makefile
# 打印info信息
$(info print your text)
```

### 7. foreach

```makefile
# 将files设置为"a.o b.o c.o d.o"
names := a b c d
files := $(foreach n,$(names),$(n).o)
```

## 伪命令

在命令行输入`make`命令后它会自动查找当前目录下的Makefile文件执行，实际测试后发现它只会执行第一个目标。一个复杂工程项目常常会在Makefile中提供多种`make`目标，通常情况下我们参考Linux源码的Makefile规则来书写我们`make`目标，以保证可读性。

常用的伪命令及功能如下：

| 伪命令         | 功能                                                   |
| -------------- | ------------------------------------------------------ |
| `make all`     | 编译所有的目标                                         |
| `make clean`   | 清除所有被`make`创建的文件                             |
| `make install` | 安装已编译好的程序，就是把目标文件拷贝到指定的目标中去 |
| `make print`   | 列出改变过的源文件                                     |
| `make tar`     | 把程序打包备份，也就是一个`tar`文件                    |
| `make dist`    | 创建一个压缩文件，一般是把`tar`文件压成`Z`或者`gz`文件 |
| `make test`    | 用于测试`makefile`的流程                               |

## 函数

常使用的函数包括`wildcard`、`notdir`和`patsubst`

## 实例

源代码：

```c++
// main.cpp
#include <iostream>
#include "header.h"

int main()
{
    printf("now start thrift server and client...\n");
    server();
    client();
}

// server.cpp
#include <iostream>
#include "header.h"

void server(){
    printf("thrift server init successfully!\n");
}

// client.cpp
#include <iostream>
#include "header.h"

void client(){
    printf("thrift client init successfully!\n");
}
```

头文件：

```c++
// header.h
void server();
void client();
```

### 1. g++编译

```bash
g++ main.cpp client.cpp server.cpp -o make_test
./make_test
```

输出结果：

```bash
now start thrift server and client...
thrift server init successfully!
thrift client init successfully!
```

### 2. Makefile

最简单的Makefile文件就是直接翻译一行g++命令，这样做的劣势在于当有一个依赖文件更新时，相当于整个项目都要重新编译：

```bash
make_test: main.cpp server.cpp client.cpp
    gcc main.cpp server.cpp client.cpp -o make_test
```

将不同的源文件拆开编译后可以解决上述劣势，但是Makefile的代码量也剧增：

```bash
make_test: main.o server.o client.o
    gcc main.o server.o client.o -o make_test

main.o: main.cpp
    gcc -c main.cpp -o main.o

server.o: server.cpp
    gcc -c server.cpp -o server.o

client.o: client.cpp
    gcc -c client.cpp -o client.o
```

我们使用变量简化makefile代码，但只是简化了重复的部分，如果有多个目标文件会导致`obj`变量很长。

```bash
obj = main.o server.o client.o
target = make_test
CC = gcc

$(target): $(obj)
    $(CC) $(obj) -o $(target)

# 表示所有的.o文件依赖于对应的.c文件
%.o: %.c
    $(CC) -c $< -o $@
```

使用函数自动搜索目录下所有的目标文件：

```bash
CXX = g++
CXXFLAGS = -Wall -std=c++11 -g

# 寻找当前目录下的所有.cpp文件
src = $(wildcard ./*.cpp)
# 将所有.cpp文件替换为.o文件生成obj
obj = $(patsubst %.cpp, %.o, $(src))
# 与上式相同的功能
# obj = $(src:%.cpp=%.o)
target = make_test

$(target): $(obj)
    $(CXX) $(obj) -o $(target)

# 模式规则：如果需要.o文件，则自动根据依赖的.cpp文件生成
%.o: %.cpp
    $(CXX) -c $< -o $@ ${CXXFLAGS}

# 删除中间目标文件和最终可执行文件
clean:
    rm -rf $(obj) $(target)
```

执行完之后通过`make clean`命令可以删除所有的中间目标文件和最终目标文件

## demo

### 1. 编译C++项目

```makefile
objDir=obj
binDir=bin
srcDir=src

CXX = g++
CXXFLAGS=-Wall -std=c++11 -g

mkdir:
    mkdir obj/ bin/
clean:
    rm -r obj/ bin/

VPATH += ${srcDir} # 设置%.cpp寻找路径
VPATH += ${objDir} # 设置%.o寻找路径

%.o : %.cpp
    ${CXX} -c $< -o ${objDir}/$@ ${CXXFLAGS}

.PHONY: code1
code1: code1.o
    ${CXX} $< -o ${binDir}/$@.out
    ./${binDir}/$@.out

.PHONY: code2
code2: code2.o
    ${CXX} $< -o ${binDir}/$@.out
```

### 2. 编译C语言项目

```makefile
edit : main.o kbd.o command.o display.o 
    cc -o edit main.o kbd.o command.o display.o

main.o : main.c defs.h
    cc -c main.c
kbd.o : kbd.c defs.h command.h
    cc -c kbd.c
command.o : command.c defs.h command.h
    cc -c command.c
display.o : display.c defs.h
    cc -c display.c

clean :
     rm edit main.o kbd.o command.o display.o

.PHONY: edit clean
```

## Reference

[1] <https://www.zhihu.com/question/23792247/answer/600773044>

[2] <https://www.cnblogs.com/mjk961/p/7862170.html>

[3] <https://blog.csdn.net/zong596568821xp/article/details/81134406>

[4] <https://zhuanlan.zhihu.com/p/56489231>

[5] <http://www.ruanyifeng.com/blog/2015/02/make.html>

[6] <https://blog.csdn.net/electrocrazy/article/details/79348357>
