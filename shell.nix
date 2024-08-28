# shell.nix
{ pkgs ? import <nixpkgs> {} }:

let
  monado = pkgs.callPackage ./monado.nix {
    serviceSupport = true;
    gst-plugins-base = pkgs.gst_all_1.gst-plugins-base;
    gstreamer = pkgs.gst_all_1.gstreamer;
  };

  nsBuildMonadoIncremental = pkgs.writeShellScriptBin "nsBuildMonadoIncremental" ''
    #!/usr/bin/env bash
    set -e

    mkdir -p build
    cd build

    if [ ! -f CMakeCache.txt ]; then
      cmake .. \
        -DXRT_FEATURE_SERVICE=ON \
        -DXRT_OPENXR_INSTALL_ABSOLUTE_RUNTIME_PATH=ON \
        -DXRT_BUILD_DRIVER_SIMULAVR=ON \
        -DXRT_HAVE_XVISIO=ON
    fi

    cmake --build . -- -j$(nproc)

    ln -sf "$(pwd)/src/xrt/targets/service/monado-service" ../monado-service

    echo "Monado incremental build completed successfully"
  '';

  rmBuilds = pkgs.writeShellScriptBin "rmBuilds" ''
    #!/usr/bin/env bash
    set -e

    find . -name build -type d -exec rm -rf {} +

    echo "All build directories have been cleaned."
  '';

in
pkgs.mkShell {
  buildInputs = monado.buildInputs ++ monado.nativeBuildInputs ++ [
    nsBuildMonadoIncremental rmBuilds
  ] ++ (with pkgs; [
    cmake
    pkg-config
    python3
  ]);

  shellHook = ''
    echo "Monado development environment loaded.."
    echo "Use 'nsBuildMonadoIncremental' to perform an incremental build.."
  '';
}
