SRC = ../app/src/
INCS_PATH = -I../app/inc
OBJS = main.o \
	   MyClientSocket.o \
	   EventActionSet.o \
	   AsyncEvents.o \
	   AisleManage.o \
	   Gpr.o \
	   MyPublicFunction.o \
	   uart_api.o
CROSS_CC = arm-linux-gcc
PC_CC = gcc
CC = ${PC_CC}

APP_NAME = Rice_Seedling_Monitor

${APP_NAME}:${OBJS}
	${CC} -o ../out/${APP_NAME} $^ -lpthread
	
%.o:${SRC}%.c
	${CC} -c ${INCS_PATH} $< -o $@
	
.PHONY : clean
clean:
	-rm -rf ${OBJS} ../out/${APP_NAME}
