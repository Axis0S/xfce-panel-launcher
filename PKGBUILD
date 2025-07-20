# Maintainer: Kamil 'Novik' Nowicki <novik@axisos.org>
# GPG Key: A1929394DC6BC602A765495562F091E70A090877 (keyserver.ubuntu.com)

pkgname=xfce-panel-launcher
pkgver=0.3
pkgrel=1
pkgdesc="Full-screen application launcher plugin for XFCE panel, similar to macOS Launchpad"
arch=('x86_64' 'i686')
url="https://github.com/Axis0S/xfce-panel-launcher"
license=('GPL3')
depends=('xfce4-panel' 'gtk3' 'libxfce4util' 'glib2')
makedepends=('pkg-config' 'gcc' 'make')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')
validpgpkeys=('A1929394DC6BC602A765495562F091E70A090877')  # Kamil 'Novik' Nowicki

build() {
    cd "$srcdir/$pkgname-$pkgver"
    make
}

package() {
    cd "$srcdir/$pkgname-$pkgver"
    
    # Create directories
    install -dm755 "$pkgdir/usr/lib/xfce4/panel/plugins"
    install -dm755 "$pkgdir/usr/share/xfce4/panel/plugins"
    
    # Install plugin library
    install -Dm755 libxfcelauncher.so "$pkgdir/usr/lib/xfce4/panel/plugins/libxfcelauncher.so"
    
    # Install desktop file
    install -Dm644 xfce-launcher.desktop "$pkgdir/usr/share/xfce4/panel/plugins/xfce-launcher.desktop"
    
    # Install documentation
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
