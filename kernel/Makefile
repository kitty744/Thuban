kernel_srcs := $(shell find kernel -name '*.c' -o -name '*.s')
objs-y += $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(kernel_srcs)))