AETHER_COMPILER = ../../../build/aetherc
CC = gcc
CFLAGS = -O3 -march=native -flto -I../../../compiler -I../../../runtime -I../../../runtime/actors -I../../../runtime/scheduler -I../../../runtime/config -I../../../runtime/utils -I../../../std -I../../../std/string -I../../../std/io -I../../../std/math -I../../../std/net -I../../../std/collections -I../../../std/json
LDFLAGS = -pthread -lm -flto
RUNTIME_SRCS = ../../../runtime/scheduler/multicore_scheduler.c \
               ../../../runtime/aether_numa.c \
               ../../../runtime/aether_runtime.c \
               ../../../runtime/config/aether_optimization_config.c \
               ../../../runtime/utils/aether_cpu_detect.c \
               ../../../runtime/actors/aether_send_buffer.c \
               ../../../runtime/actors/aether_send_message.c \
               ../../../runtime/actors/aether_actor_thread.c

ping_pong: ping_pong.ae
	@echo "Compiling Aether source..."
	@$(AETHER_COMPILER) ping_pong.ae /tmp/ping_pong_aether.c
	@echo "Compiling to native binary..."
	@$(CC) $(CFLAGS) /tmp/ping_pong_aether.c $(RUNTIME_SRCS) -o ping_pong $(LDFLAGS)
	@echo "Done: ./ping_pong"

clean:
	rm -f ping_pong /tmp/ping_pong_aether.c*

.PHONY: clean
