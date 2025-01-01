#!/bin/bash

main() {
    local -r workarea='neotetris_dependencies_install'

    pushd /tmp
    mkdir "${workarea}"
    pushd "${workarea}"

    git clone https://github.com/pottumuusi/vulkan-tools.git

    ./vulkan-tools/scripts/install_glslc.sh

    popd # ${workarea}
    rm -rf "${workarea}"

    popd # /tmp
}

main "${@}"