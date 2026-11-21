#!/usr/bin/env bash
# Run by build.cmd after the cartridge has been validated.
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:$PATH"
target=$GE_BUILD_TARGET
packages=(mingw-w64-x86_64-gcc)
if [[ $target != xbox ]]; then
    packages+=(mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-SDL2
               mingw-w64-x86_64-zlib mingw-w64-x86_64-python mingw-w64-x86_64-binutils)
fi
if [[ $target != windows ]]; then

    packages+=(make cmake bison flex python mingw-w64-x86_64-clang
               mingw-w64-x86_64-lld mingw-w64-x86_64-llvm mingw-w64-x86_64-llvm-tools)
fi
if ! pacman -T "${packages[@]}"; then
    echo 'MSYS2 needs additional packages. Review its update before continuing.'
    echo 'A core update can close MSYS2 terminals. If prompted to restart, rerun build.cmd.'
    pacman -Syu --needed "${packages[@]}"
fi

if [[ $target != xbox ]]; then
    echo 'Building GoldenEye for Windows...'
    bash ./build-pc.sh ntsc-final
    for pass in d43_emit d69_emit; do
        /mingw64/bin/python "tools_pc/$pass.py" ntsc-final
    done
    /mingw64/bin/python tools_pc/d88_emit.py ntsc-final --regen
    out=build-pc/play
    mkdir -p "$out/data"
    cp build-pc/ge007.x86_64.exe "$out/"
    cp -R data/ge007.ntsc-final.z64 data/pcmodels-ntsc-final data/pccg-ntsc-final "$out/data/"

    # Walk PE imports so a player can launch from Explorer without a MinGW PATH.
    shopt -s nocasematch
    pending=("$out/ge007.x86_64.exe")
    declare -A copied=()
    while ((${#pending[@]})); do
        file=${pending[-1]}
        unset 'pending[-1]'
        imports=$(objdump -p "$file" | awk '/DLL Name:/ {sub(/\r$/, "", $3); print $3}')
        while IFS= read -r dll; do
            [[ -n $dll && -f /mingw64/bin/$dll ]] || continue
            key=${dll,,}
            [[ -z ${copied[$key]:-} ]] || continue
            cp "/mingw64/bin/$dll" "$out/"
            copied[$key]=1
            pending+=("$out/$dll")
        done <<< "$imports"
    done
fi

if [[ $target != windows ]]; then
    if [[ -n ${NXDK_DIR:-} ]]; then
        NXDK_DIR=$(cygpath -au "$NXDK_DIR")
    else
        read -r repo rev destination < tools_pc/nxdk-sources.lock
        NXDK_DIR="/opt/goldeneye/nxdk-$rev"
        mkdir -p "$NXDK_DIR"
        # Each marker is written only after its pinned archive extracts fully.
        # A retry resumes at the first incomplete dependency.
        curl="$(cygpath -u "$SYSTEMROOT")/System32/curl.exe"
        while read -r repo rev destination; do
            marker="$NXDK_DIR/.source-$rev"
            [[ -f $marker ]] && continue
            archive=$(mktemp)
            trap 'rm -f "$archive"' EXIT
            echo "Downloading $repo..."
            "$curl" --fail --location --retry 3 --silent --show-error \
                --output "$(cygpath -aw "$archive")" "https://codeload.github.com/$repo/tar.gz/$rev"
            mkdir -p "$NXDK_DIR/$destination"
            exclusions=(--exclude="*/Xcode" --exclude="*/Xcode-iOS")
            # libc++ ships intentionally dangling symlinks in its filesystem tests.
            if [[ $destination == lib/libcxx ]]; then exclusions+=(--exclude="*/test"); fi
            tar -xzf "$archive" "${exclusions[@]}" --strip-components=1 -C "$NXDK_DIR/$destination"
            touch "$marker"
            rm -f "$archive"
            trap - EXIT
        done < tools_pc/nxdk-sources.lock
    fi
    if [[ ! -f $NXDK_DIR/share/toolchain-nxdk.cmake || $(cygpath -aw "$NXDK_DIR") == *' '* ]]; then
        echo 'nxdk needs a valid SDK checkout in a path without spaces. Check NXDK_DIR and your MSYS2 location.' >&2
        exit 1
    fi
    export NXDK_DIR
    export PATH="$NXDK_DIR/bin:/usr/bin:/mingw64/bin:$PATH"
    export CMAKE_GENERATOR='Unix Makefiles'
    if [[ ! -f "$NXDK_DIR/lib/libc++.lib" || ! -x "$NXDK_DIR/tools/extract-xiso/build/extract-xiso" || ! -x "$NXDK_DIR/tools/cxbe/cxbe" ]]; then
        make -C "$NXDK_DIR" -j"$(nproc)" NXDK_ONLY=y NXDK_SDL=y NXDK_NET=y all cxbe extract-xiso
    fi
    /usr/bin/python3 port/xbox/build.py --rom data/ge007.ntsc-final.z64
fi
