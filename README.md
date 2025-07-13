# Secuenciador - MBed CE

Proyecto Final de Graduación - Lucas Gutiérrez - 2025
Facultad de Ciencias Exactas y Tecnología - Universidad Nacional de Tucumán

## How to set up this project:

1. Clone it to your machine. Don't forget to use `--recursive` to clone the submodules: `git clone --recursive https://github.com/LucastuFett/SecuenciadorMBed.git`
2. You may want to update the mbed-os submodule to the latest version, with `cd mbed-ce-hello-world/mbed-os && git fetch origin && git reset --hard origin/master`
3. Set up the GNU ARM toolchain (and other programs) on your machine using [the toolchain setup guide](https://github.com/mbed-ce/mbed-os/wiki/Toolchain-Setup-Guide).
4. Set up the CMake project for editing. We have three ways to do this:
   - On the [command line](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-Command-Line)
   - Using the [CLion IDE](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-CLion)
   - Using the [VS Code IDE](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-VS-Code)
5. Download and setup [picotool](https://github.com/raspberrypi/picotool)
6. Build the `flash-Secuenciador` target to upload the code to a connected device.
