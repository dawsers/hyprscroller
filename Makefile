.PHONY: all debug release clean install dev

debug:
	cmake -B ./Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$(PREFIX)
	cmake --build ./Debug -j
	rm -rf ./compile_commands.json
	rm -rf ./hyprscroller.so
	ln -s ./Debug/compile_commands.json .
	ln -s ./Debug/hyprscroller.so .

release:
	cmake -B ./Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(PREFIX)
	cmake --build ./Release -j
	rm -rf ./compile_commands.json
	rm -rf ./hyprscroller.so
	ln -s ./Release/compile_commands.json .
	ln -s ./Release/hyprscroller.so .

all: clean release

clean:
	rm -rf Release
	rm -rf Debug
	rm -rf ./hyprscroller.so
	rm -rf ./compile_commands.json

install: release
	mkdir -p `xdg-user-dir`/.config/hypr/plugins
	cp hyprscroller.so `xdg-user-dir`/.config/hypr/plugins

dev: clean debug
