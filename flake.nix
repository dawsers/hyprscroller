{
  description = "hyprscroller";

  inputs.hyprland.url = "github:hyprwm/Hyprland";

  outputs =
    { self, hyprland }:
    let
      inherit (hyprland.inputs) nixpkgs;
      inherit (nixpkgs) lib;

      allSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems =
        func:
        lib.genAttrs allSystems (
          system:
          let
            pkgs = nixpkgs.legacyPackages.${system};
            hyprlandPkg = hyprland.packages.${system}.hyprland;
            hyprscrollerPkg = self.packages.${system}.hyprscroller;
          in
          func { inherit pkgs hyprlandPkg hyprscrollerPkg; }
        );

      inherit (builtins) substring;
      mkDate =
        date:
        (lib.concatStringsSep "-" [
          (substring 0 4 date)
          (substring 4 2 date)
          (substring 6 2 date)
        ]);
      version = "date=${mkDate (self.lastModifiedDate or "19700101")}_${self.shortRev or "dirty"}";
    in
    {
      packages = forAllSystems (
        {
          pkgs,
          hyprlandPkg,
          hyprscrollerPkg,
        }:
        {
          hyprscroller = pkgs.stdenv.mkDerivation {
            pname = "hyprscroller";
            inherit version;
            src = self;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkg-config
              hyprlandPkg
            ];
            buildInputs = [ hyprlandPkg ] ++ hyprlandPkg.buildInputs;

            buildPhase = ''
              make all
            '';

            installPhase = ''
              mkdir -p $out/lib
              cp ./hyprscroller.so $out/lib/libhyprscroller.so
            '';

            meta = with lib; {
              homepage = "https://github.com/dawsers/hyprscroller";
              description = "Hyprland layout plugin providing a scrolling layout like PaperWM";
              license = licenses.mit;
              platforms = platforms.linux;
            };
          };
          default = hyprscrollerPkg;
        }
      );

      devShells = forAllSystems (
        {
          pkgs,
          hyprlandPkg,
          hyprscrollerPkg,
        }:
        {
          default = pkgs.mkShell {
            shellHook = ''
              make dev
            '';
            name = "hyprscroller-shell";
            nativeBuildInputs = with pkgs; [ cmake ];
            buildInputs = [ hyprlandPkg ];
            inputsFrom = [
              hyprlandPkg
              hyprscrollerPkg
            ];
          };
        }
      );

      formatter = forAllSystems ({ pkgs, ... }: pkgs.nixfmt-rfc-style);
    };
}
