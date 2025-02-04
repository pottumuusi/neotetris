#!/bin/bash

readonly INSTALL_SDL="FALSE"

main() {
    local -r workarea='neotetris_dependencies_install'

    pushd /tmp
    mkdir "${workarea}"
    pushd "${workarea}"

    if [ "TRUE" == "$INSTALL_SDL" ] ; then
        sudo apt install -y libx11-dev libxext-dev

        wget https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.24.0.zip
        unzip release-2.24.0.zip
        pushd SDL-release-2.24.0
        ./configure
        make
        sudo make install
        popd
    fi

    git clone https://github.com/pottumuusi/vulkan-tools.git

    ./vulkan-tools/scripts/install_glslc.sh

    # Create containing directory for Vulkan SDK.
    mkdir -p ${HOME}/my/tools

    ./vulkan-tools/scripts/install_vulkan_sdk.sh

    popd # ${workarea}
    rm -rf "${workarea}"

    popd # /tmp
}

main "${@}"