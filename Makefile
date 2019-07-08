.PHONY = build clean list-deps run zig-build zig-list-deps zig-run FORCE

build: bin Makefile main.c
	cc main.c `pkg-config --libs --cflags raylib` -o bin/space-sim

# build-static:
# 	cc libraylib-2.5.0.a main.c \
# 	    -o space-sim \
# 	    -framework Cocoa -framework CoreVideo\
# 	    -framework GLUT-framework IOKit -framework OpenGL \
# 	    -Xlinker -w

bin:
	@mkdir -p bin

clean:
	@rm -rf bin zig-cache

list-deps: build
	@otool -L bin/space-sim

run: build
	@./bin/space-sim

zig-build: FORCE
	@zig build

zig-list-deps:
	@zig build install --prefix .
	@otool -L bin/space-sim

zig-run: FORCE
	@zig build run

FORCE:

