lib_srcs := $(shell find lib -name '*.c' -o -name '*.s')
objs-y += $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(lib_srcs)))