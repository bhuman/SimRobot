CONFIG ?= Release

.PHONY: all clean

all:
	@cmake --build ../../Build/Linux/CMake/$(CONFIG) --config $(CONFIG)

clean:
	@cmake --build ../../Build/Linux/CMake/$(CONFIG) --target clean --config $(CONFIG)

%:
	@cmake --build ../../Build/Linux/CMake/$(CONFIG) --target $@ --config $(CONFIG)
