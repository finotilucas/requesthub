#!/usr/bin/env bash
#
# Build a self-contained RequestHub AppImage.
#
set -euo pipefail

# --- paths & config --------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

ARCH="${ARCH:-$(uname -m)}"
VERSION="${VERSION:-dev}"
TOOLS_DIR="${TOOLS_DIR:-${ROOT}/build/appimage-tools}"
APPDIR="${ROOT}/build/AppDir"
BUILD_DIR="${ROOT}/build"
OUTPUT="${OUTPUT:-RequestHub-${VERSION}-${ARCH}.AppImage}"

APP_ID="io.github.finotilucas.requesthub"
BINARY="${BUILD_DIR}/release/requesthub"

LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
GTK_PLUGIN_URL="https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy.AppImage"
GTK_PLUGIN="${TOOLS_DIR}/linuxdeploy-plugin-gtk.sh"
LD_EXTRACTED="${TOOLS_DIR}/linuxdeploy-extracted"
APPIMAGETOOL="${LD_EXTRACTED}/plugins/linuxdeploy-plugin-appimage/usr/bin/appimagetool"

log() { printf '\033[1;34m[appimage]\033[0m %s\n' "$*"; }
die() { printf '\033[1;31m[appimage]\033[0m %s\n' "$*" >&2; exit 1; }

# --- 1. preflight & build --------------------------------------------------

for cmd in wget make pkg-config find cmp install; do
  command -v "${cmd}" >/dev/null || die "missing required command: ${cmd}"
done
for f in "${SCRIPT_DIR}/${APP_ID}.desktop" \
         "${SCRIPT_DIR}/${APP_ID}.metainfo.xml" \
         "${ROOT}/assets/icons/${APP_ID}.svg"; do
  [[ -f "${f}" ]] || die "missing asset: ${f}"
done

log "building release binary"
make -C "${ROOT}" release
[[ -x "${BINARY}" ]] || die "release binary missing after build: ${BINARY}"

# --- 2. fetch & extract tools ----------------------------------------------

mkdir -p "${TOOLS_DIR}"

if [[ ! -x "${LINUXDEPLOY}" ]]; then
  log "fetching linuxdeploy"
  wget -q -O "${LINUXDEPLOY}" "${LINUXDEPLOY_URL}"
  chmod +x "${LINUXDEPLOY}"
fi

if [[ ! -x "${GTK_PLUGIN}" ]]; then
  log "fetching linuxdeploy-plugin-gtk"
  wget -q -O "${GTK_PLUGIN}" "${GTK_PLUGIN_URL}"
  chmod +x "${GTK_PLUGIN}"
  # Modern GTK4 (Arch/Void/Fedora) no longer ships /usr/lib/gtk-4.0/;
  # IM modules and print backends are linked into libgtk-4 itself.
  sed -i \
    's#copy_lib_tree "\$gtk4_libdir" "\$APPDIR/"#if [ -d "$gtk4_libdir" ]; then copy_lib_tree "$gtk4_libdir" "$APPDIR/"; fi#' \
    "${GTK_PLUGIN}"
fi

if [[ ! -x "${APPIMAGETOOL}" ]]; then
  log "extracting linuxdeploy (for appimagetool)"
  rm -rf "${LD_EXTRACTED}"
  ( cd "${TOOLS_DIR}" && rm -rf squashfs-root && "${LINUXDEPLOY}" --appimage-extract >/dev/null )
  mv "${TOOLS_DIR}/squashfs-root" "${LD_EXTRACTED}"
fi

# --- 3. PATH shims for plugin's child processes ----------------------------

SHIMS_DIR="${TOOLS_DIR}/shims"
mkdir -p "${SHIMS_DIR}"
for t in glib-compile-schemas gdk-pixbuf-query-loaders gio-querymodules gtk-update-icon-cache; do
  real="$(PATH=/usr/bin:/usr/local/bin:/bin command -v "${t}" || true)"
  [[ -z "${real}" ]] && continue
  cat > "${SHIMS_DIR}/${t}" <<EOF
#!/bin/sh
unset LD_LIBRARY_PATH
exec "${real}" "\$@"
EOF
  chmod +x "${SHIMS_DIR}/${t}"
done

# --- 4. stage AppDir -------------------------------------------------------

log "staging AppDir"
rm -rf "${APPDIR}"
install -Dm0755 "${BINARY}" "${APPDIR}/usr/bin/requesthub"
install -Dm0644 "${SCRIPT_DIR}/${APP_ID}.desktop" \
                "${APPDIR}/usr/share/applications/${APP_ID}.desktop"
install -Dm0644 "${SCRIPT_DIR}/${APP_ID}.metainfo.xml" \
                "${APPDIR}/usr/share/metainfo/${APP_ID}.metainfo.xml"
install -Dm0644 "${ROOT}/assets/icons/${APP_ID}.svg" \
                "${APPDIR}/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"

# --- 5. deploy with linuxdeploy + gtk plugin -------------------------------

log "running linuxdeploy + GTK plugin"
export DEPLOY_GTK_VERSION=4
export APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}"
export PATH="${SHIMS_DIR}:${TOOLS_DIR}:${PATH}"

"${LINUXDEPLOY}" \
  --appdir "${APPDIR}" \
  --executable "${APPDIR}/usr/bin/requesthub" \
  --desktop-file "${APPDIR}/usr/share/applications/${APP_ID}.desktop" \
  --icon-file "${APPDIR}/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" \
  --plugin gtk

# --- 6. resync bundled libs with host versions -----------------------------

log "resyncing bundled libs with host versions"
libdir="${APPDIR}/usr/lib"
replaced=0
while IFS= read -r -d '' lib; do
  name="$(basename "${lib}")"
  host="/usr/lib/${name}"
  [[ -f "${host}" ]] || continue
  cmp -s "${host}" "${lib}" && continue
  cp -L --preserve=mode "${host}" "${lib}"
  replaced=$((replaced + 1))
done < <(find "${libdir}" -maxdepth 1 -name '*.so*' -not -type l -print0)
log "resynced ${replaced} bundled libraries"

for round in 1 2 3 4 5; do
  added=0
  while IFS= read -r -d '' lib; do
    while IFS= read -r dep; do
      dep="${dep##* }"
      [[ -z "${dep}" || -e "${libdir}/${dep}" ]] && continue
      hostdep="/usr/lib/${dep}"
      [[ -f "${hostdep}" ]] || continue
      cp -L --preserve=mode "${hostdep}" "${libdir}/${dep}"
      added=$((added + 1))
    done < <(objdump -p "${lib}" 2>/dev/null | grep '^  NEEDED')
  done < <(find "${libdir}" -maxdepth 1 -name '*.so*' -not -type l -print0)
  log "closure round ${round}: pulled ${added} additional libs"
  [[ "${added}" -eq 0 ]] && break
  [[ "${round}" -eq 5 ]] && die "lib closure did not converge"
done

# --- 7. patch linuxdeploy-plugin-gtk hook ----------------------------------
#
# The generated AppRun only sources apprun-hooks/linuxdeploy-plugin-gtk.sh
# (extra files in apprun-hooks/ are ignored). That plugin queries the desktop
# portal + GSettings for color-scheme and falls back to GTK_THEME=Adwaita:light
# when nothing answers (typical on KDE / non-GNOME hosts). GTK_THEME is honored
# by GTK at startup, so the app boots in light mode before AdwStyleManager can
# override, producing a visually broken Adwaita+light/dark mix.
#
# We append our pins at the END of the hook so they win when the env is
# exported. We also force the Adwaita icon theme so the bundled Adwaita icons
# beat the user's gtk-icon-theme-name=breeze-dark / Papirus / etc.

gtk_hook="${APPDIR}/apprun-hooks/linuxdeploy-plugin-gtk.sh"
if [[ -f "${gtk_hook}" ]]; then
  log "patching plugin-gtk hook to force Adwaita:dark"
  cat >> "${gtk_hook}" <<'EOF'

# --- requesthub overrides (appended by build-appimage.sh) ------------------
# Pin the libadwaita color scheme; the plugin's portal/gsettings detection
# defaults to light on KDE/Sway/etc., which fights AdwStyleManager(FORCE_DARK).
export GTK_THEME="Adwaita:dark"
# Match icon theme to the bundled assets (we ship /usr/share/icons/Adwaita
# inside the AppImage; without this the host's gtk-icon-theme-name wins).
export GTK_ICON_THEME_NAME="Adwaita"
# The plugin sets GDK_BACKEND=x11 to dodge an old GTK4-on-Wayland crash that
# no longer reproduces on current GTK4 + libadwaita. Forcing x11 means
# Wayland users get XWayland, which can misrender HiDPI scaling and degrade
# input. Clear it so GTK4 picks the user's native backend ($WAYLAND_DISPLAY
# → wayland, $DISPLAY → x11). Users hitting an edge-case crash can still
# set GDK_BACKEND=x11 manually before launching the AppImage.
unset GDK_BACKEND
# --- end requesthub overrides ----------------------------------------------
EOF
else
  log "warning: linuxdeploy-plugin-gtk.sh hook not found; theme may misrender"
fi

# --- 8. bundle GtkSourceView-5 data ----------------------------------------

gsv="$(pkg-config --variable=datadir gtksourceview-5 2>/dev/null || echo /usr/share)/gtksourceview-5"
if [[ -d "${gsv}" ]]; then
  log "bundling GtkSourceView-5 data"
  mkdir -p "${APPDIR}/usr/share/gtksourceview-5"
  cp -r "${gsv}/." "${APPDIR}/usr/share/gtksourceview-5/"
fi

# --- 9. bundle Adwaita icon theme ------------------------------------------

adwaita_icons="/usr/share/icons/Adwaita"
if [[ -d "${adwaita_icons}" ]]; then
  log "bundling Adwaita icon theme"
  mkdir -p "${APPDIR}/usr/share/icons"
  cp -rL "${adwaita_icons}" "${APPDIR}/usr/share/icons/Adwaita"
  # Refresh the icon cache so GTK doesn't fall back to per-file scans.
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f "${APPDIR}/usr/share/icons/Adwaita" || true
  fi
else
  log "warning: Adwaita icon theme not found at ${adwaita_icons}; AppImage"
  log "         will fall back to the host's icon theme at runtime"
fi

# --- 10. refresh pixbuf loaders cache --------------------------------------

pixbuf_cache="${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
pixbuf_loaders="${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders"
if [[ -d "${pixbuf_loaders}" && -x "${SHIMS_DIR}/gdk-pixbuf-query-loaders" ]]; then
  log "regenerating gdk-pixbuf loaders.cache"
  GDK_PIXBUF_MODULEDIR="${pixbuf_loaders}" \
    "${SHIMS_DIR}/gdk-pixbuf-query-loaders" > "${pixbuf_cache}"
  sed -i "s|${pixbuf_loaders}/||g" "${pixbuf_cache}"
fi

# --- 11. package via appimagetool ------------------------------------------

log "packaging ${OUTPUT}"
rm -f "${BUILD_DIR}/${OUTPUT}"
ARCH="${ARCH}" "${APPIMAGETOOL}" "${APPDIR}" "${BUILD_DIR}/${OUTPUT}"

# --- 12. diagnostic summary ------------------------------------------------

log "build summary:"
log "  AppDir size:       $(du -sh "${APPDIR}" | cut -f1)"
log "  AppImage size:     $(du -sh "${BUILD_DIR}/${OUTPUT}" | cut -f1)"
adw_so="$(find "${APPDIR}" -name 'libadwaita-1.so*' -print -quit 2>/dev/null)"
log "  libadwaita:        ${adw_so:-NOT BUNDLED}"
gtk_so="$(find "${APPDIR}" -name 'libgtk-4.so*' -print -quit 2>/dev/null)"
log "  libgtk-4:          ${gtk_so:-NOT BUNDLED}"
if [[ -d "${APPDIR}/usr/share/icons/Adwaita" ]]; then
  log "  Adwaita icons:     $(du -sh "${APPDIR}/usr/share/icons/Adwaita" | cut -f1)"
else
  log "  Adwaita icons:     NOT BUNDLED"
fi
schemas_count=$(find "${APPDIR}/usr/share/glib-2.0/schemas" -name '*.xml' 2>/dev/null | wc -l)
log "  GSettings schemas: ${schemas_count} files"
theme_line="$(grep -E '^export GTK_THEME=' "${APPDIR}/apprun-hooks/linuxdeploy-plugin-gtk.sh" 2>/dev/null | tail -1)"
log "  GTK_THEME (final): ${theme_line:-NOT SET}"

log "done: ${BUILD_DIR}/${OUTPUT}"
