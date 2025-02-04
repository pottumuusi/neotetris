#!/bin/bash

main() {
    local -r workarea='neotetris_dependencies_install'

    pushd /tmp
    mkdir "${workarea}"
    pushd "${workarea}"

    git clone https://github.com/pottumuusi/vulkan-tools.git

    ./vulkan-tools/scripts/install_glslc.sh
    mkdir -p ${HOME}/my/tools
    ./vulkan-tools/scripts/install_vulkan_sdk.sh

    popd # ${workarea}
    rm -rf "${workarea}"

    popd # /tmp
}

main "${@}"