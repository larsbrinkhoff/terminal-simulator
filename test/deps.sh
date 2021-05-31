install_ubuntu() {
    sudo apt-get update -ym
    sudo apt-get install -y libsdl2-dev libsdl2-image-dev
}

install_macos() {
    brew update
    brew install sdl2 sdl2_image
}

case `uname` in
    Linux) install_ubuntu;;
    Darwin) install_macos;;
    *) echo "Unknown operating system"; exit 1;;
esac
