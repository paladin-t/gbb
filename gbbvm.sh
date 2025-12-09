#!/bin/bash
echo "Building, a moment please..."

# Prepare environment.
if [[ "$OSTYPE" == "darwin"* ]]; then
  gbdk="tools/gbdk/mac/bin"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
  gbdk="tools/gbdk/linux/bin"
else
  echo "Unsupported OS: $OSTYPE"
  exit 1
fi

# Colorizer.
color_echo() {
  local color=$1
  local message=$2
  local reset='\033[0m'

  case $color in
    "2F")
      echo -e "\033[42;37m${message}${reset}"
      ;;
    "3F")
      echo -e "\033[46;37m${message}${reset}"
      ;;
    *)
      echo -e "${message}"
      ;;
  esac
}

# To clear build cache only.
if [ "$1" == "clear" ]; then
  echo "Cleaning build artifacts..."
  rm -rf "output/vm"
  rm -f "gbbasic/kernels/gbbvm.gb" "gbbasic/kernels/gbbvm.sym"
  echo "Clean done."
  exit 0
fi

# Get the build options.
rebuild=1
if [ "$1" = "/y" ]; then
  echo "Rebuild all."
elif [ "$1" = "/n" ]; then
  rebuild=0
  echo "Not rebuild all."
else
  read -p "Rebuild all? [y/n] " i
  if [ "$i" != "y" ]; then
    rebuild=0
  fi
fi

withcrashhandler=1

# Set the variables.
main="main"
bootstrap="bootstrap"
builtin="builtin"

macros="-DVM_TOTAL_CONTEXT_STACK_SIZE=1024 -DVM_MAX_CONTEXTS=16 -DVM_HEAP_SIZE=1024"
cflag="-Wf\"--opt-code-speed\" $macros -Wf-Isrc/vm -Wa-Isrc/vm -Wa-Isrc/vm/drv/hUGE/player-gbdk"

libs="-Wl-lsrc/vm/drv/hUGE/lib/hUGEDriver.lib -Wl-ksrc/vm/drv/hUGE/lib"
carty="-Wl-yt0x1b -Wm-yc"
nbanks="-Wl-yo16 -Wm-ya16 -autobank"
symbols="-Wl-j -Wl-m -Wl-w -Wm-yS"
lflag="-Wf\"--opt-code-speed\" $libs $carty $nbanks $symbols"

# Build.
color_echo "2F" "Building..."
mkdir -p "output/vm"

if [ $rebuild -eq 1 ]; then
  color_echo "3F" "1. Compile the libs..."
  "$gbdk/lcc" $cflag -c -o "output/vm/exception.o" "src/vm/utils/exception.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/font.o" "src/vm/utils/font.s"
  "$gbdk/lcc" $cflag -c -o "output/vm/graphics.o" "src/vm/utils/graphics.s"
  "$gbdk/lcc" $cflag -c -o "output/vm/scroll.o" "src/vm/utils/scroll.s"
  "$gbdk/lcc" $cflag -c -o "output/vm/sfx_player.o" "src/vm/utils/sfx_player.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/sgb.o" "src/vm/utils/sgb.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/sleep.o" "src/vm/utils/sleep.s"
  "$gbdk/lcc" $cflag -c -o "output/vm/text.o" "src/vm/utils/text.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/timer_handler.o" "src/vm/utils/timer_handler.s"
  "$gbdk/lcc" $cflag -c -o "output/vm/utils.o" "src/vm/utils/utils.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/utils_banked.o" "src/vm/utils/utils_banked.c"

  if [ $withcrashhandler -eq 1 ]; then
    "$gbdk/lcc" $cflag -c -o "output/vm/crash_handler.o" "src/vm/utils/crash_handler.s"
  fi
  echo "Ok."
  echo
else
  color_echo "3F" "1. Ignore compiling the libs..."
  echo "Ok."
  echo
fi

if [ $rebuild -eq 1 ]; then
  color_echo "3F" "2. Compile the controllers..."
  "$gbdk/lcc" $cflag -c -o "output/vm/controller.o" "src/vm/ctrl/controller.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/navigation.o" "src/vm/ctrl/navigation.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/platformer.o" "src/vm/ctrl/platformer.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/pointnclick.o" "src/vm/ctrl/pointnclick.c"
  "$gbdk/lcc" $cflag -c -o "output/vm/topdown.o" "src/vm/ctrl/topdown.c"
  echo "Ok."
  echo
else
  color_echo "3F" "2. Ignore compiling the controllers..."
  echo "Ok."
  echo
fi

color_echo "3F" "3. Compile the VM..."
set -x
"$gbdk/lcc" $cflag -c -o "output/vm/vm.o" "src/vm/vm.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_actor.o" "src/vm/vm_actor.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_actor_aux.o" "src/vm/vm_actor_aux.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_audio.o" "src/vm/vm_audio.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_device.o" "src/vm/vm_device.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_device_ext.o" "src/vm/vm_device_ext.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_effects.o" "src/vm/vm_effects.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_emote.o" "src/vm/vm_emote.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_game.o" "src/vm/vm_game.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_graphics.o" "src/vm/vm_graphics.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_gui.o" "src/vm/vm_gui.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_gui_label.o" "src/vm/vm_gui_label.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_gui_menu.o" "src/vm/vm_gui_menu.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_gui_progressbar.o" "src/vm/vm_gui_progressbar.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_input.o" "src/vm/vm_input.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_memory.o" "src/vm/vm_memory.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_native.o" "src/vm/vm_native.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_object.o" "src/vm/vm_object.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_persistence.o" "src/vm/vm_persistence.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_physics.o" "src/vm/vm_physics.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_projectile.o" "src/vm/vm_projectile.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_scene.o" "src/vm/vm_scene.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_scroll.o" "src/vm/vm_scroll.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_serial.o" "src/vm/vm_serial.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_system.o" "src/vm/vm_system.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_trigger.o" "src/vm/vm_trigger.c"
"$gbdk/lcc" $cflag -c -o "output/vm/vm_instructions.o" "src/vm/vm_instructions.c"
"$gbdk/lcc" $cflag -c -o "output/vm/${main}.o" "src/vm/${main}.c"
set +x
echo "Ok."
echo

if [ $rebuild -eq 1 ]; then
  color_echo "3F" "4. Compile the data..."
  "$gbdk/lcc" $cflag -c -o "output/vm/${bootstrap}.o" "src/vm/data/${bootstrap}.s"
  "$gbdk/lcc" $cflag -c -o "output/vm/${builtin}.o" "src/vm/data/${builtin}.c"
  echo "Ok."
  echo
else
  color_echo "3F" "4. Ignore compiling the data..."
  echo "Ok."
  echo
fi

color_echo "3F" "5. Link..."
libobj=(
  "output/vm/exception.o"
  "output/vm/font.o"
  "output/vm/graphics.o"
  "output/vm/scroll.o"
  "output/vm/sfx_player.o"
  "output/vm/sgb.o"
  "output/vm/sleep.o"
  "output/vm/text.o"
  "output/vm/timer_handler.o"
  "output/vm/utils.o"
  "output/vm/utils_banked.o"
)

if [ $withcrashhandler -eq 1 ]; then
  libobj+=("output/vm/crash_handler.o")
fi

ctrlobj=(
  "output/vm/controller.o"
  "output/vm/navigation.o"
  "output/vm/platformer.o"
  "output/vm/pointnclick.o"
  "output/vm/topdown.o"
)

dataobj=(
  "output/vm/${bootstrap}.o"
  "output/vm/${builtin}.o"
)

set -x
"$gbdk/lcc" $lflag -o "output/vm/gbbvm.gb" \
  "${libobj[@]}" \
  "${ctrlobj[@]}" \
  "output/vm/vm.o" \
  "output/vm/vm_actor.o" \
  "output/vm/vm_actor_aux.o" \
  "output/vm/vm_audio.o" \
  "output/vm/vm_device.o" \
  "output/vm/vm_device_ext.o" \
  "output/vm/vm_effects.o" \
  "output/vm/vm_emote.o" \
  "output/vm/vm_game.o" \
  "output/vm/vm_graphics.o" \
  "output/vm/vm_gui.o" \
  "output/vm/vm_gui_label.o" \
  "output/vm/vm_gui_menu.o" \
  "output/vm/vm_gui_progressbar.o" \
  "output/vm/vm_input.o" \
  "output/vm/vm_memory.o" \
  "output/vm/vm_native.o" \
  "output/vm/vm_object.o" \
  "output/vm/vm_persistence.o" \
  "output/vm/vm_physics.o" \
  "output/vm/vm_projectile.o" \
  "output/vm/vm_scene.o" \
  "output/vm/vm_scroll.o" \
  "output/vm/vm_serial.o" \
  "output/vm/vm_system.o" \
  "output/vm/vm_trigger.o" \
  "output/vm/vm_instructions.o" \
  "output/vm/${main}.o" \
  "${dataobj[@]}"
set +x
echo "Ok."
echo

# Copy.
color_echo "2F" "Copying..."
if [ -f "output/vm/gbbvm.gb" ]; then
  mkdir -p "gbbasic/kernels"
  cp "output/vm/gbbvm.gb" "gbbasic/kernels/"
  cp "output/vm/gbbvm.sym" "gbbasic/kernels/"
  echo "Copied successfully."
else
  echo "gbbvm.gb doesn't exist."
fi
echo "Ok."
echo

# Finish.
echo "Building done!"
exit 0
