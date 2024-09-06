{
  description = "hyprscroller";

  inputs.hyprland = {
    type = "git";
    url = "https://github.com/hyprwm/Hyprland";
    submodules = true;
  };

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
      rawCommitPins = (builtins.fromTOML (builtins.readFile ./hyprpm.toml)).repository.commit_pins;
      commitPins = builtins.listToAttrs (
        map (p: {
          name = builtins.head p;
          value = builtins.elemAt p 1;
        }) rawCommitPins
      );
      selfRev = "${commitPins.${hyprland.rev} or "git"}";
      selfShortRev = substring 0 7 selfRev;
      version = "date=${
        mkDate (self.lastModifiedDate or "19700101")
      }_${self.shortRev or "dirty"}_${selfShortRev}";
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
            src =
              if (commitPins ? ${hyprland.rev}) && (self ? rev) then
                (builtins.fetchGit {
                  url = "https://github.com/dawsers/hyprscroller";
                  rev = selfRev;
                })
              else
                ./.;

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
