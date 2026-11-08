# Repackage existing application binaries; never compile or replace their SDL here.
# This runtime advertises Linux >= 4.4. Keep the conventional build separate.
FROM archlinux:base@sha256:b1c2fe6e717bfd5673af8fc217e1e1c3ce0dcbb1d53c3414c8425e7ce79fc6d9

# Freeze the dependency repositories along with the tested mini Mesa 26.2.6 assets.
# A changed/deleted upstream release asset must fail its checksum, not silently
# introduce another Mesa or libc into the release.
RUN printf '%s\n' 'Server = https://archive.archlinux.org/repos/2026/11/08/$repo/os/$arch' > /etc/pacman.d/mirrorlist \
 && pacman -Syu --noconfirm --needed \
    patchelf binutils curl ca-certificates xorg-server-xvfb xorg-xauth \
    fontconfig dbus libxcrypt-compat libglvnd mesa vulkan-icd-loader \
    libpulse alsa-lib libusb libdecor pipewire sndio jack2 \
    libxrandr libxcursor libxi libxinerama libxss libxkbcommon wayland \
    libjpeg-turbo libarchive zstd \
 && pacman -Scc --noconfirm

COPY mesa-packages.sha256 /tmp/mesa-packages.sha256
RUN cd /tmp \
 && while read -r checksum package; do \
      curl -fL --retry 3 -o "$package" "https://github.com/pkgforge-dev/archlinux-pkgs-debloated/releases/download/continuous/$package"; \
    done < mesa-packages.sha256 \
 && sha256sum -c mesa-packages.sha256 \
 && pacman -U --noconfirm ./*-mini-x86_64.pkg.tar.zst \
 && rm -f ./*.pkg.tar.zst /tmp/mesa-packages.sha256

WORKDIR /tmp
ENV HOME=/tmp
