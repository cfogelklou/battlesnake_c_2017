
.PHONY: all
all: build

.PHONY: build
build:
	cd build && \
		cmake .. -DCMAKE_BUILD_TYPE=Debug && \
		make

.PHONY: build_opt
build_opt:
	cd build && \
		cmake .. -DCMAKE_BUILD_TYPE=Optimize && \
		make


.PHONY: clean
clean:
	rm -rf build/*

