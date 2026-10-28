#!/system/bin/sh
# run.sh — su -c "sh /data/local/tmp/run.sh [-i /path/lib.so]"

PKG="FILL.package.name"                      # FILL: pm list packages | grep -i delta
BIN="/data/local/tmp/esp"                    # FILL: your compiled overlay binary
INJECTOR="/data/local/tmp/injector"          # FILL: ptrace injector binary, usage: injector <pid> <lib.so>
LIB=""

while [ $# -gt 0 ]; do
  case "$1" in
    -i) LIB="$2"; shift 2 ;;
    *)  shift ;;
  esac
done

[ "$(id -u)" -eq 0 ] || { echo "need root"; exit 1; }
[ -x "$BIN" ] || { echo "no $BIN (chmod 755)"; exit 1; }

getpid() {
  for d in /proc/[0-9]*; do
    read -r c < "$d/cmdline" 2>/dev/null || continue
    [ "${c%%$(printf '\0')*}" = "$PKG" ] && { echo "${d#/proc/}"; return; }
  done
}

OLD_SE=$(getenforce)
setenforce 0

cleanup() {
  [ -n "$ESP_PID" ] && kill "$ESP_PID" 2>/dev/null
  [ "$OLD_SE" = "Enforcing" ] && setenforce 1
  exit 0
}
trap cleanup INT TERM EXIT

# launch game if not running
PID=$(getpid)
if [ -z "$PID" ]; then
  monkey -p "$PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1
  echo "waiting for $PKG..."
  i=0
  while [ -z "$PID" ] && [ $i -lt 120 ]; do
    sleep 1; i=$((i+1)); PID=$(getpid)
  done
fi
[ -n "$PID" ] || { echo "game not found"; exit 1; }
echo "pid $PID"

# wait for engine lib to be mapped before touching anything
i=0
until grep -q "libUE4.so" "/proc/$PID/maps" 2>/dev/null; do
  sleep 1; i=$((i+1)); [ $i -gt 90 ] && { echo "libUE4 not mapped"; exit 1; }
done
sleep 5

# inject (optional)
if [ -n "$LIB" ]; then
  [ -x "$INJECTOR" ] || { echo "no injector"; exit 1; }
  "$INJECTOR" "$PID" "$LIB" || { echo "inject failed"; exit 1; }
fi

# overlay
"$BIN" "$PKG" &
ESP_PID=$!
echo "esp up, pid $ESP_PID, ctrl+c to stop"
wait "$ESP_PID"
