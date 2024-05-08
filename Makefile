##############################################################################
# 编译工具
CROSS_COMPILE ?=

AS		    = $(CROSS_COMPILE)as
LD		    = $(CROSS_COMPILE)ld
CC		    = $(CROSS_COMPILE)gcc
CPP		    = $(CC) -E
AR		    = $(CROSS_COMPILE)ar
NM		    = $(CROSS_COMPILE)nm

##############################################################################
#目标文件
TARGET = fcgi-cell

#生成文件夹
BUILD_DIR = obj

#源文件
SRC_PATH = src 
SRC_DIR = $(shell find $(SRC_PATH) -maxdepth 0 -type d)
SOURCES = $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))

EASYLOGGER_SRC_PATH = src/easylogger 
EASYLOGGER_SRC_DIR = $(shell find $(EASYLOGGER_SRC_PATH) -maxdepth 0 -type d)
EASYLOGGER_SOURCES = $(foreach dir, $(EASYLOGGER_SRC_DIR), $(wildcard $(dir)/*.c))

#头文件
INC_PATH = include
INC_DIR := $(INC_PATH)
INC_DIR += $(SRC_PATH)

#库文件路径
LIB_PATH = lib
LIB_DIR = $(shell find $(LIB_PATH) -maxdepth 1 -type d)
LIBS = $(foreach dir, $(LIB_DIR), $(wildcard $(dir)/lib*))

#编译头文件选项
CFLAGS := -Wall
CFLAGS += $(addprefix -I,${INC_DIR})

#编译库选项
LDFLAGS= -L$(LIB_DIR) -lpthread -lm -lcjson -lfcgi -lsqlite3
#所有依赖文件的路径名列表
OBJS=$(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

##############################################################################
#编译目标
all: default $(TARGET)

#$(TARGET) : $(OBJS)
#	$(CC) $^ -o $@ $(LDFLAGS)

#编译.o文件
#$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
#	$(CC) -c $< -o $@ $(CFLAGS)

$(TARGET):$(SOURCES) $(EASYLOGGER_SOURCES)
	$(CC) -o $@ $^ $(INC_DIRS) $(CXXFLAGS) $(CFLAGS) $(LIBS) $(LDFLAGS)

#伪目标（清除生成文件、创建生成文件夹）
.PHONY:all clean help default

default:
	rm -rf $(TARGET)
#	mkdir -p $(BUILD_DIR)

clean:
#	rm -rf $(BUILD_DIR)
	rm -rf $(TARGET)


help:
	@echo '******'
	@echo '生成文件   :' $(TARGET)
	@echo '源码文件   :' $(SOURCES)
	@echo '头文件目录   :' $(INC_DIR)
	@echo '编译依赖   :' $(CFLAGS)
	@echo '库依赖  :' $(LDFLAGS)
	@echo '编译文件  :' $(OBJS)
	@echo '******'
