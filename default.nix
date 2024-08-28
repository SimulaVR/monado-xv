let
  pkgs = (import <nixpkgs> { });
in
pkgs.callPackage ./monado.nix { gst-plugins-base = pkgs.gst_all_1.gst-plugins-base; gstreamer = pkgs.gst_all_1.gstreamer; }

