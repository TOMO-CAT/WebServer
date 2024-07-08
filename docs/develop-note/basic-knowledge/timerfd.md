# timerfd

## 简介

timerfd 指的是定时器 fd，它的可读可写事件与时间相关。timerfd 被 new 出来之后（`timerfd_create`）后可以设置超时时间（`timerfd_settime`），超时之后该句柄可读，读出来的是**超时次数**。

以 Linux 机器上一个 open 了的 timerfd 进程为例：

```bash
root@ubuntu:~# ll /proc/6997/fd/
...
lrwx------ 1 root root 64 Aug 10 14:13 3 -> anon_inode:[timerfd]

root@ubuntu:~# cat /proc/6997/fdinfo/3 
pos: 0
flags: 02
mnt_id: 11
clockid: 0
ticks: 0
settime flags: 01
it_value: (0, 969820149)
it_interval: (1, 0)
```

通过 `/proc/${pid}/fd/` 可以查看进程打开的句柄，其中 `anon_inode:[timerfd]` 表示**timerfd 绑定的是匿名 inode**。

通过 `/proc/${pid}/fdinfo` 可以查看句柄的展示信息：

* `clockid`：时钟类型
* `ticks`：超时次数
* `setting_flags`：这是 `timerfd_settime` 的参数
* `it_value`：定时器到期还剩多少时间
* `it_interval`：超时间隔

文件句柄、网络句柄都是支持 `read`、`write` 和 `close` 的，timerfd 可以 `read`、`poll` 和 `close`，这从内核实现的接口也可以看出来：

```c
// fs/timerfd.c
static const struct file_operations timerfd_fops = { 
    .release    = timerfd_release,
    .poll       = timerfd_poll,
    .read       = timerfd_read,
    .show_fdinfo    = timerfd_show,
    // ...
};
```

其中 `timerfd_show` 展示的信息就是我们上面看到的 `cat /proc/${pid}/fdinfo/` 里的 timerfd 信息。

## 相关的系统调用

涉及到 timerfd 的系统调用有三个，函数原型如下：

```c
// 创建一个 timerfd 句柄
int timerfd_create(int clockid, int flags);

// 启动或关闭 timerfd 对应的定时器
int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

// 获取指定 timerfd 距离下一次超时还剩的时间
int timerfd_gettime(int fd, struct itimerspec *curr_value);
```

timerfd 常用来做定时器的使用，设置超时时间之后，每隔一段时间 timerfd 就是可读的。使用 man timerfd_create 就能查看到完整的文档，有一个 c 语言的示例，简要看下这个例子：

```c
int main(int argc, char *argv[]) {
    // 第一次超时时间
    new_value.it_value.tv_sec = now.tv_sec + atoi(argv[1]);
    new_value.it_value.tv_nsec = now.tv_nsec;

    // 设置超时间隔
    new_value.it_interval.tv_sec = atoi(argv[2]);
    new_value.it_interval.tv_nsec = 0;

    // 创建 timerfd
    fd = timerfd_create(CLOCK_REALTIME, 0);
    // 设置第一次超时时间和超时间隔
    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
    // 定时器循环
    for (tot_exp = 0; tot_exp < max_exp;) {
        // read timerfd，获取到超时次数
        s = read(fd, &exp, sizeof(uint64_t));
        // 累计总超时次数
        tot_exp += exp;
        // 打印超时次数的信息
        printf("read: %llu; total=%llu\n", (unsigned long long) exp, (unsigned long long) tot_exp);
    }
}
```

在这个例子中：

* 通过 `timerfd_create` 获取到一个句柄后，使用 `timerfd_settime` 设置超时时间并启动内核定时器
* 后续使用 `read` 来读取数据，timerfd 没超时之前 read 会阻塞，直到内核定时器超时之后 read 才会返回，这样就实现了一个定时器效果

程序运行之后相当于每隔一段时间 sleep 一下，然后打印一行信息，周期性运行，这就是 timerfd 官方最简单的例子。

`timerfd` 可以和 epoll 配合，让 epoll 监听 timerfd 的可读事件，这样 timerfd 超时触发可读事件，`epoll_wait` 被唤醒，业务进行周期处理。

## Reference

[1] <https://zhuanlan.zhihu.com/p/409434419>
