# 定义变量
CC = gcc
CFLAGS = -I./inc -Wall -Wextra
STATIC_CFLAGS = -I./inc -Wall -Wextra -static
SRCDIR = src
INCDIR = inc
BINDIR = bin
RADIR = ra
VSOCKDIR = vsock
BUILDDIR = build
BUSYBOX_URL = https://busybox.net/downloads/busybox-1.36.1.tar.bz2
BUSYBOX_DIR = tools/busybox-1.36.1
BUSYBOX_BIN = $(BUSYBOX_DIR)/busybox
SEVCTL_REPO = https://github.com/virtee/sevctl
SEVCTL_DIR = tools/sevctl

RASOURCES = $(wildcard $(RADIR)/*.c)
RATARGETS = $(RASOURCES:$(RADIR)/%.c=$(BINDIR)/%)
VSOCKSOURCES = $(wildcard $(VSOCKDIR)/*.c)
VSOCKTARGETS = $(VSOCKSOURCES:$(VSOCKDIR)/%.c=$(BINDIR)/%)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BINDIR)/%.o)
TARGET = shelter

CONTAINER_NAME = container_shelter
IMAGE_NAME = shelter

GRPC_GUEST_DIR = grpc_guest


# 默认目标
all: $(TARGET) ra vsock grpc_guest

# 编译目标
$(TARGET): $(OBJECTS)
	mkdir -p $(BUILDDIR)
	$(CC) $(filter-out $(BINDIR)/main.o,$(OBJECTS)) $(BINDIR)/main.o -o $@

# 编译每个源文件
$(BINDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 ra 目录下的程序为可执行文件
ra: $(RATARGETS)

$(BINDIR)/%: $(RADIR)/%.c
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

# 编译 vsock 目录下的程序为可执行文件
vsock: $(VSOCKTARGETS)

$(BINDIR)/%: $(VSOCKDIR)/%.c
	@mkdir -p $(BINDIR)
	$(CC) $(STATIC_CFLAGS) $< -o $@

# 编译 grpc_guest 目录下的 Rust 项目
grpc_guest:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(GRPC_GUEST_DIR)/src/generated
	RUSTFLAGS="-Z macro-backtrace"
	cd $(GRPC_GUEST_DIR) && cargo build --release --target x86_64-unknown-linux-musl


# 下载并编译 BusyBox、SEVCTL
tools: $(BUSYBOX_BIN) $(SEVCTL_DIR)/target/release/sevctl

$(BUSYBOX_BIN):
	@mkdir -p tools
	wget -O tools/busybox.tar.bz2 $(BUSYBOX_URL)
	tar -xjf tools/busybox.tar.bz2 -C tools
	cd $(BUSYBOX_DIR) && make defconfig
	sed -i 's/.*CONFIG_STATIC.*/CONFIG_STATIC=y/' $(BUSYBOX_DIR)/.config
	sed -i 's/.*CONFIG_TC=y/CONFIG_TC=n/' $(BUSYBOX_DIR)/.config
	cd $(BUSYBOX_DIR) && make -j$(nproc)

$(SEVCTL_DIR)/target/release/sevctl:
	@mkdir -p tools
	git clone $(SEVCTL_REPO) $(SEVCTL_DIR)
	cd $(SEVCTL_DIR) && rustup target add x86_64-unknown-linux-musl
	cd $(SEVCTL_DIR) && cargo build --release --target x86_64-unknown-linux-musl

# 清理目标
clean:
	rm -rf $(BINDIR) $(TARGET)
	rm -rf $(BUILDDIR)
	rm -f ra/secret_*
	rm -f ra/sev_*
	rm -f ra/measurement.bin
	rm -f ra/secret.txt
	rm -f ra/sev.pdh
	rm -rf grpc_guest/src/generated/*
	-docker rm -f $(CONTAINER_NAME)
	-docker rmi -f $(IMAGE_NAME)
	cd $(GRPC_GUEST_DIR) && cargo clean
	cd $(GRPC_GUEST_DIR) && rm -f Cargo.lock

.PHONY: all clean ra vsock tools grpc_guest
 