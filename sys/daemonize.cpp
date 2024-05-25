#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>

int daemonize()
{
	pid_t pid;
	//因为我们从shell创建的daemon子进程，所以daemon子进程会继承shell的umask，如果不清除的话，会导致daemon进程创建文件时屏蔽某些权限。
    umask(0);

	//fork后让父进程退出，子进程获得新的pid，肯定不为进程组组长，这是setsid前提。
	if ((pid = fork()) < 0){
		return -1;
	}
	else if (pid != 0){
		exit(0);
	}

	//调用setsid来创建新的进程会话。这使得daemon进程成为会话首进程，脱离和terminal的关联。
    if (setsid() == -1){
        return -2;
    }

	//最好在这里再次fork。这样使得daemon进程不再是会话首进程，那么永远没有机会获得控制终端。如果这里不fork的话，会话首进程依然可能打开控制终端。
	if ((pid = fork()) < 0){
		return -3;
	}
	else if (pid != 0){
		exit(0);
	}

	//将当前工作目录切换到根目录。父进程继承过来的当前目录可能mount在一个文件系统上，如果不切换到根目录，那么这个文件系统不允许unmount。
    chdir("/");

	//在子进程中关闭从父进程中继承过来的那些不需要的文件描述符。可以通过_SC_OPEN_MAX来判断最高文件描述符(不是很必须).
	struct rlimit rl;
	getrlimit(RLIMIT_NOFILE, &rl);
	if (rl.rlim_max == RLIM_INFINITY){
		rl.rlim_max = 1024;
	}
	for (int i = 0; i < rl.rlim_max; i++){
		close(i);
	}

	//打开/dev/null复制到0,1,2，因为dameon进程已经和terminal脱离了，所以需要重新定向标准输入，标准输出和标准错误(不是很必须).
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        return -4;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        return -5;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        return -6;
    }

    if (dup2(fd, STDERR_FILENO) == -1) {
	    return -7;
    }
	
	if (fd > STDERR_FILENO) {
		//释放资源这样这个文件描述符就可以被复用，不影响STDIN_FILENO和STDOUT_FILENO，因为dup2已经把STDIN_FILENO和STDOUT_FILENO指向了/dev/null。
		if (close(fd) == -1) {
			return -8;
		}
	}

    return 0;
}