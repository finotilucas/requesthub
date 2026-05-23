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

# --- 7. AppRun hook: LD_LIBRARY_PATH ---------------------------------------

cat > "${APPDIR}/apprun-hooks/00-library-path.sh" <<'EOF'
APPDIR="${APPDIR:-"$(dirname "$(realpath "$0")")"}"
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
EOF

# --- 8. bundle GtkSourceView-5 data ----------------------------------------

gsv="$(pkg-config --variable=datadir gtksourceview-5 2>/dev/null || echo /usr/share)/gtksourceview-5"
if [[ -d "${gsv}" ]]; then
  log "bundling GtkSourceView-5 data"
  mkdir -p "${APPDIR}/usr/share/gtksourceview-5"
  cp -r "${gsv}/." "${APPDIR}/usr/share/gtksourceview-5/"
fi

# --- 9. refresh pixbuf loaders cache ---------------------------------------

pixbuf_cache="${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
pixbuf_loaders="${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders"
if [[ -d "${pixbuf_loaders}" && -x "${SHIMS_DIR}/gdk-pixbuf-query-loaders" ]]; then
  log "regenerating gdk-pixbuf loaders.cache"
  GDK_PIXBUF_MODULEDIR="${pixbuf_loaders}" \
    "${SHIMS_DIR}/gdk-pixbuf-query-loaders" > "${pixbuf_cache}"
  sed -i "s|${pixbuf_loaders}/||g" "${pixbuf_cache}"
fi

# --- 10. package via appimagetool ------------------------------------------

log "packaging ${OUTPUT}"
rm -f "${BUILD_DIR}/${OUTPUT}"
ARCH="${ARCH}" "${APPIMAGETOOL}" "${APPDIR}" "${BUILD_DIR}/${OUTPUT}"

log "done: ${BUILD_DIR}/${OUTPUT}"
