WORK_DIR ?= $(shell pwd)
# build: $(SUBDIRS) build-kubos build-img

.PHONY:build
build: 
	docker build -f build-kubos/Dockerfile . -t build-kubos:build-img

.PHONY:run
run:
	sudo docker run --rm -it -v ${WORK_DIR}/kubos-linux/:/kubos-linux/ build-kubos:build-img

# .PHONY:clean
# clean:
# 	docker run --rm -it -v ${WORK_DIR}/kubos-linux/:/kubos-linux/ build-kubos:build-img "cd /kubos-linux/buildroot-2019.02.2; && make clean"