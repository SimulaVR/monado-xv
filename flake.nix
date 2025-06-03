{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/63dacb46bf939521bdc93981b4cbb7ecb58427a0";
    systems.url = "github:nix-systems/x86_64-linux";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } {
      systems = import inputs.systems;

      imports = [ inputs.treefmt-nix.flakeModule ];

      perSystem =
        { pkgs, system, ... }:
        let
          monado-xv = pkgs.callPackage ./. { };
          xvsdk = pkgs.callPackage ./submodules/xvsdk/xvsdk.nix { }; # submodules dependency
        in
        {
          _module.args.pkgs = import inputs.nixpkgs {
            inherit system;
            config.allowUnfree = true;
          };

          treefmt = {
            projectRootFile = "flake.nix";
            programs.nixfmt.enable = true;
          };

          packages = {
            inherit monado-xv;
            default = monado-xv;
          };

          devShells.default = pkgs.mkShell rec {
            nativeBuildInputs = [
              # Development tools
              pkgs.nil

              # Build tools
              pkgs.just
              pkgs.cmake
              pkgs.doxygen
              pkgs.glslang
              pkgs.pkg-config
              pkgs.python3
            ];

            buildInputs = [
              pkgs.bluez
              pkgs.cjson
              pkgs.dbus
              pkgs.eigen
              pkgs.elfutils
              pkgs.ffmpeg
              pkgs.gst_all_1.gst-plugins-base
              pkgs.gst_all_1.gstreamer
              pkgs.hidapi
              pkgs.libbsd
              pkgs.libdrm
              pkgs.libffi
              pkgs.libGL
              pkgs.libjpeg
              pkgs.librealsense
              pkgs.libsurvive
              pkgs.libunwind
              pkgs.libusb1
              pkgs.libuv
              pkgs.libuvc
              pkgs.libv4l
              pkgs.xorg.libXau
              pkgs.xorg.libxcb
              pkgs.xorg.libXdmcp
              pkgs.xorg.libXext
              pkgs.xorg.libXrandr
              pkgs.onnxruntime
              pkgs.opencv4
              pkgs.openhmd
              pkgs.openvr
              pkgs.orc
              pkgs.pcre2
              pkgs.SDL2
              pkgs.shaderc
              pkgs.udev
              pkgs.vulkan-headers
              pkgs.vulkan-loader
              pkgs.wayland
              pkgs.wayland-protocols
              pkgs.wayland-scanner
              pkgs.zlib
              pkgs.zstd

              xvsdk
            ];

            # Environment variables to pass cmake through just command
            XVSDK_INCLUDE_DIR = "${xvsdk}/include";
            XVSDK_LIBRARY_DIR = "${xvsdk}/lib";

            shellHook = ''
              export PS1="\n[nix-shell:\w]$ "
            '';
          };
        };
    };
}
