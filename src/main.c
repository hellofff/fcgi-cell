#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <unistd.h>

#include <easylogger/elog.h>
#include <fcgi2/fcgi_stdio.h>
#include <cjson/cJSON.h>
#include "cellular_serial.h"
#include "cellular_yanfei.h"
#include "version.h"

#define DEVICE_SERIAL "/dev/ttyUSB2"
/*AT发送超时定义*/

static void _elog_config(void);
static void check_pid_file(char *appname);
void fcgi_cell(void);


/*
    main函数
*/
int main()
{

    _elog_config();
    log_i("software version:%d.%d.%d",MAJOR_VERSION,MINIOR_VERSION,PATH_VERSION);

    check_pid_file("fcgi-cell");
	//yanfei_run();
	fcgi_cell();
	//yanfei_upline_at();
	//yanfei_upline_ati();
			
    return 0;
}

/*
    配置elog
*/
static void _elog_config(void)
{
    setbuf(stdout, NULL);
    /* initialize EasyLogger */
    elog_init();
    /* set EasyLogger log format */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO,ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG,ELOG_FMT_DIR|ELOG_FMT_LINE|ELOG_FMT_TIME|ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);

#ifdef ELOG_COLOR_ENABLE
    elog_set_text_color_enabled(true);
#endif
    /* start EasyLogger */
    elog_start();
}

/*
    检测进程的PID文件
*/
static void check_pid_file(char *appname)
{
	//char pid_file[] = "/var/run/tsn-manage.pid";
    char pid_file[40] = {0};
	char str[20] = { 0 };
	int ret = 0;
	int fd;

    sprintf(pid_file,"/var/run/%s.pid",appname);
	/* open PID file */
	fd = open(pid_file, O_RDWR | O_CREAT, 0640);
	if (fd < 0) {
		log_i("Unable to open %s PID file '%s': %s.\n",
		       appname,pid_file, strerror(errno));
		exit(1);
	}

	/* acquire lock on the PID file */
	if (lockf(fd, F_TLOCK, 0) < 0) {
		if (EACCES == errno || EAGAIN == errno) {
			log_i("Another instance of %s %s\n",
			       appname,"daemon is running, unable to start.");
		} else {
			log_i("Unable to lock %s PID file '%s': %s.",
			       appname,pid_file, strerror(errno));
		}
		close(fd);
		exit(1);
	}

	/* write PID into the PID file */
	snprintf(str, 20, "%d\n", getpid());
	ret = write(fd, str, strlen(str));
	if (-1 == ret) {
		log_i("ERR: Unable to write into %s PID file '%s': %s.",
		       appname,pid_file, strerror(errno));
		close(fd);
		exit(1);
	}

	close(fd);
}
