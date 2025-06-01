build:
    mkdir -p build
    cd build; cmake .. \
        -DXRT_FEATURE_SERVICE=ON \
        -DXRT_OPENXR_INSTALL_ABSOLUTE_RUNTIME_PATH=ON \
        -DXRT_BUILD_DRIVER_SIMULAVR=ON \
        -DXRT_HAVE_XVISIO=ON \
        -DXRT_HAVE_LIBUVC=OFF \
        -DXVSDK_INCLUDE_DIR=$XVSDK_INCLUDE_DIR \
        -DXVSDK_LIBRARY_DIR=$XVSDK_LIBRARY_DIR

    cmake --build ./build -- -j$(nproc)

    ln -sf build/src/xrt/targets/service/monado-service .

clean:
    rm -rf build/
    rm -f monado-service
