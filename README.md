# PGGK EASY CROSSPLAY

PGGK Easy Crossplay (PGGKEC) is a study project I am developing to facilitate crossplay between retro consoles using UDP Sockets with some reliability options. PGGKEC works as a single header and is currently adapted to work on the following platforms:
- [x] Windows
- [x] Linux
- [x] 3DS
- [x] PSVita
- [x] PSP
- [ ] PS2
- [ ] NDS
- [ ] Wii
- [ ] WiiU
- [x] Switch
- [ ] Android

## Requirements
Currently, the setup is performed only on a Windows environment, complemented by WSL2 with Ubuntu.
I have not yet tested whether it works with pure Linux.

To compile the examples, you need to have the following requirements installed:

All
- [MinGW 32](https://sourceforge.net/projects/mingw/)
- [Ubuntu WSL2](https://documentation.ubuntu.com/wsl/en/latest/guides/install-ubuntu-wsl2/)

3DS and Switch
- [DevkitPro Toolchain](https://github.com/devkitPro/installer/releases) (Make sure to enable the correct platforms)

PSP (Install these requirements using WSL2)
- [PSPDEV Toolchain](https://github.com/pspdev/psptoolchain-allegrex)
- [PSPSDK](https://github.com/pspdev/pspsdk)

PSVita
- [VitaSDK](https://vitasdk.org/)

Ensure that the installed Ubuntu is set as the default in WSL2 using `wsl --list`. If it is not, you need to change it with `wsl --set-default <Distribution Name>`.