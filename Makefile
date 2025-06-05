# -*- Makefile -*-

CC = gcc
DBG_CC = gcc-13
CFLAGS = -g -fsanitize=address -fsanitize=leak -Wall -Wextra
SOURCES = src/main.c src/util.c src/tab_complete.c src/fuzzy_finder.c src/readline.c
CURRENT_DIR = $(shell pwd)
INSTALLDIR ?= $(CURRENT_DIR)/bin  # 默认安装到项目下的bin目录
BIN_DIR = $(CURRENT_DIR)/bin
DEBUG_BIN = $(BIN_DIR)/zsh-debug
PROD_BIN = $(BIN_DIR)/zsh

.SILENT:
prod: ${SOURCES} ## 编译生产版本shell
	@mkdir -p $(BIN_DIR)  # 确保bin目录存在
	${CC} ${SOURCES} -o ${PROD_BIN} -ldl -lm
	@echo "生产版本已编译到: ${PROD_BIN}"

.SILENT:
debug: ${SOURCES} ## 编译调试版本shell
	@mkdir -p $(BIN_DIR)  # 确保bin目录存在
	${DBG_CC} ${CFLAGS} ${SOURCES} -o ${DEBUG_BIN} -ldl -lm
	@echo "调试版本已编译到: ${DEBUG_BIN}"

.SILENT:
compile_and_run_debug:
	@make debug
	${DEBUG_BIN}

help:  ## 显示帮助信息
	@awk 'BEGIN {FS = ":.*##"; printf "\n用法:\n  make \033[36m<目标>\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

docker_shell: ${SOURCES}
	@mkdir -p $(BIN_DIR)
	${CC} ${SOURCES} -o ${PROD_BIN} -ldl -lm


integration_tests: ## 需要先运行 'docker build -t testing_container .'
	make clean
	docker run -ti --rm -v $(CURRENT_DIR):/pshell testing_container \
		"make docker_shell && cat ./tests/integration_tests/autocomplete_tests.txt >> /root/.psh_history && ruby ./tests/integration_tests/test_pshell.rb"
	make clean

install: prod ## 安装可执行文件到系统目录
	@echo "安装到: ${INSTALLDIR} (使用 sudo 获取权限)"
	sudo mkdir -p $(INSTALLDIR)
	sudo cp $(PROD_BIN) $(INSTALLDIR)
	@echo "安装成功! 可执行文件位置: $(INSTALLDIR)/psh"

user_install: prod ## 安装到用户目录 (~/.local/bin)
	@mkdir -p $(HOME)/.local/bin
	cp $(PROD_BIN) $(HOME)/.local/bin/psh
	@echo "安装成功! 可执行文件位置: $(HOME)/.local/bin/psh"
	@echo "请确保 $(HOME)/.local/bin 在您的 PATH 环境变量中"

clean: ## 清理所有二进制文件
	rm -rf $(BIN_DIR)/*
	@echo "已清理所有生成的文件"