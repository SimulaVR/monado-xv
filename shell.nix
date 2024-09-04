# shell.nix
{ pkgs ? import <nixpkgs> {} }:

let
  monado = pkgs.callPackage ./monado.nix {
    serviceSupport = true;
    gst-plugins-base = pkgs.gst_all_1.gst-plugins-base;
    gstreamer = pkgs.gst_all_1.gstreamer;
  };

  xvsdkSrc = pkgs.fetchFromGitHub {
    owner = "SimulaVR";
    repo = "xvsdk";
    rev = "c58f6e022742841c8dc9a476ec80eb37416c0332";
    sha256 = "14lfh2m1zfpgqi5y6x1pkckr0gk9x9q1d33q04lgxkggm8ipprsb";
  };
  xvsdk = pkgs.callPackage (xvsdkSrc + "/xvsdk.nix") { };


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
        -DXRT_HAVE_XVISIO=ON \
        -DXVSDK_INCLUDE_DIR=${xvsdk}/include \
        -DXVSDK_LIBRARY_DIR=${xvsdk}/lib
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
    nsBuildMonadoIncremental
    rmBuilds
    xvsdk
  ] ++ (with pkgs; [
    cmake
    pkg-config
    python3
  ]);

  shellHook = ''
    echo "Monado development environment loaded.."
    echo "Use 'nsBuildMonadoIncremental' to perform an incremental build.."
    export XVSDK_INCLUDE_DIR="${xvsdk}/include"
    export XVSDK_LIBRARY_DIR="${xvsdk}/lib"
  '';
}
