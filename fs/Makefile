fs_srcs := $(shell find fs -name '*.c' -o -name '*.s')
objs-y += $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(fs_srcs)))