{
  lib,
  stdenv,
  fetchFromGitLab,
  writeText,
  bluez,
  cjson,
  cmake,
  dbus,
  doxygen,
  eigen,
  elfutils,
  ffmpeg,
  glslang,
  gst_all_1,
  hidapi,
  libbsd,
  libdrm,
  libffi,
  libGL,
  libjpeg,
  libsurvive,
  libunwind,
  libusb1,
  libuv,
  libuvc,
  libv4l,
  libXau,
  libxcb,
  libXdmcp,
  libXext,
  libXrandr,
  opencv4,
  openhmd,
  openvr,
  orc,
  pcre2,
  pkg-config,
  python3,
  SDL2,
  shaderc,
  udev,
  vulkan-headers,
  vulkan-loader,
  wayland,
  wayland-protocols,
  wayland-scanner,
  zlib,
  zstd,
  nixosTests,
  # Set as 'false' to build monado without service support, i.e. allow VR
  # applications linking against libopenxr_monado.so to use OpenXR standalone
  # instead of via the monado-service program. For more information see:
  # https://gitlab.freedesktop.org/monado/monado/-/blob/master/doc/targets.md#xrt_feature_service-disabled
  serviceSupport ? true,
  callPackage,

  # librealsense,  # Exclude to avoid ABI conflict errors
  # onnxruntime    # "
}:

let
  xvsdk = callPackage ./submodules/xvsdk/xvsdk.nix { };
in

stdenv.mkDerivation {
  pname = "monado";
  version = "unstable-2024-01-02";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    doxygen
    glslang
    pkg-config
    python3
  ];

  cmakeFlags = [
    "-DXRT_FEATURE_SERVICE=${if serviceSupport then "ON" else "OFF"}"
    "-DXRT_OPENXR_INSTALL_ABSOLUTE_RUNTIME_PATH=ON"
    "-DXRT_BUILD_DRIVER_SIMULAVR=ON"
    "-DXRT_HAVE_XVISIO=ON"
    "-DXRT_HAVE_LIBUVC=OFF" # to prevent conflicting with xvsdk wrapper
    "-DXVSDK_INCLUDE_DIR=${xvsdk}/include"
    "-DXVSDK_LIBRARY_DIR=${xvsdk}/lib"
    "-DXRT_HAVE_OPENCV=OFF"
    "-DXRT_BUILD_DRIVER_REALSENSE=OFF" # Avoid realsense + onnx runtime to avoid ABI conflict errors
    "-DXRT_HAVE_ONNXRUNTIME=OFF"       # "
  ];

  buildInputs = [
    bluez
    cjson
    dbus
    eigen
    elfutils
    ffmpeg
    gst_all_1.gst-plugins-base
    gst_all_1.gstreamer
    hidapi
    libbsd
    libdrm
    libffi
    libGL
    libjpeg
    libsurvive
    libunwind
    libusb1
    libuv
    libuvc
    libv4l
    libXau
    libxcb
    libXdmcp
    libXext
    libXrandr
    opencv4
    openhmd
    openvr
    orc
    pcre2
    SDL2
    shaderc
    udev
    vulkan-headers
    vulkan-loader
    wayland
    wayland-protocols
    wayland-scanner
    zlib
    zstd
    xvsdk

    # librealsense  # Exclude to avoid ABI conflict errors
    # onnxruntime   # "
  ];

  # known disabled drivers/features:
  #  - DRIVER_DEPTHAI - Needs depthai-core https://github.com/luxonis/depthai-core (See https://github.com/NixOS/nixpkgs/issues/292618)
  #  - DRIVER_ILLIXR - needs ILLIXR headers https://github.com/ILLIXR/ILLIXR (See https://github.com/NixOS/nixpkgs/issues/292661)
  #  - DRIVER_ULV2 - Needs proprietary Leapmotion SDK https://api.leapmotion.com/documentation/v2/unity/devguide/Leap_SDK_Overview.html (See https://github.com/NixOS/nixpkgs/issues/292624)
  #  - DRIVER_ULV5 - Needs proprietary Leapmotion SDK https://api.leapmotion.com/documentation/v2/unity/devguide/Leap_SDK_Overview.html (See https://github.com/NixOS/nixpkgs/issues/292624)

  # Help openxr-loader find this runtime
  setupHook = writeText "setup-hook" ''
    export XDG_CONFIG_DIRS=@out@/etc/xdg''${XDG_CONFIG_DIRS:+:''${XDG_CONFIG_DIRS}}
  '';

  # We manually modified the CmakeLists.txt to incorporate this patch:
  # patches = [
  # We don't have $HOME/.steam when building
  # ./force-enable-steamvr_lh.patch
  # ];

  # Expose xvsdk for shell.nix
  #passthru = {
  #  inherit xvsdk;
  #};

  #passthru.tests = {
  #  basic-service = nixosTests.monado;
  #};

  meta = {
    description = "Open source XR runtime";
    homepage = "https://github.com/SimulaVR/monado-xv";
    license = lib.licenses.boost;
    platforms = [ "x86_64-linux" ];
    mainProgram = "monado-cli";
  };
}
