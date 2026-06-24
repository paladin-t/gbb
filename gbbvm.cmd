@echo off
echo Building, a moment please...

REM Prepare environment.
SETLOCAL EnableDelayedExpansion
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set "DEL=%%a"
)

REM To clear build cache only.
if "%1"=="clear" (
  echo Cleaning build artifacts...
  if exist "output\vm" rmdir /s /q "output\vm"
  del "gbbasic\kernels\gbbvm.gb" 2>nul
  del "gbbasic\kernels\gbbvm.sym" 2>nul
  echo Clean done.
  exit /b 0
)

REM Get the build options.
set rebuild=0
if "%1"=="/y" (
  set rebuild=1
  echo Rebuild all.
  goto :BLD
) else (
  if "%1"=="/n" (
    echo Not rebuild all.
    goto :BLD
  )
)
set rebuild=1
set /p i="Rebuild all? [y/n]"
if "%i%" neq "y" set rebuild=0
:BLD
set withcrashhandler=1

REM Set the variables.
set gbdk=tools\gbdk\win\bin

set main=main
set bootstrap=bootstrap
set builtin=builtin

set macros=-DVM_TOTAL_CONTEXT_STACK_SIZE=1024 -DVM_MAX_CONTEXTS=16 -DVM_HEAP_SIZE=1024
set cflag=-Wf"--max-allocs-per-node 50000" -Wf"--opt-code-speed" %macros% -Wf-Isrc\vm -Wa-Isrc\vm -Wa-Isrc\vm\drv\hUGE\player-gbdk

set libs=-Wl-lsrc\vm\drv\hUGE\lib\hUGEDriver.lib -Wl-ksrc\vm\drv\hUGE\lib
set carty=-Wl-yt0x1b -Wm-yc
set nbanks=-Wl-yo16 -Wm-ya16 -autobank
set symbols=-Wl-j -Wl-m -Wl-w -Wm-yS
set lflag=%libs% %carty% %nbanks% %symbols%

REM Build.
call :COLOR 2F "Building..."
if not exist "output\vm" (
  mkdir "output\vm"
)
if %rebuild%==1 (
  call :COLOR 3F "1. Compile the libs..."
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\sgb_mouse.o" "src\vm\drv\mouse\sgb_mouse.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\sfx_player.o" "src\vm\drv\sfx\sfx_player.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\speech.o" "src\vm\drv\speech\speech.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\exception.o" "src\vm\utils\exception.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\font.o" "src\vm\utils\font.s"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\graphics.o" "src\vm\utils\graphics.s"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\scroll.o" "src\vm\utils\scroll.s"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\sgb.o" "src\vm\utils\sgb.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\sleep.o" "src\vm\utils\sleep.s"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\text.o" "src\vm\utils\text.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\timer_handler.o" "src\vm\utils\timer_handler.s"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\utils.o" "src\vm\utils\utils.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\utils_banked.o" "src\vm\utils\utils_banked.c"
  if %withcrashhandler%==1 (
    call %gbdk%\lcc.exe %cflag% -c -o "output\vm\crash_handler.o" "src\vm\utils\crash_handler.s"
  )
  echo Ok.
  @echo,
) else (
  call :COLOR 3F "1. Ignore compiling the libs..."
  echo Ok.
  @echo,
)

if %rebuild%==1 (
  call :COLOR 3F "2. Compile the controllers..."
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\controller.o" "src\vm\ctrl\controller.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\navigation.o" "src\vm\ctrl\navigation.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\platformer.o" "src\vm\ctrl\platformer.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\pointnclick.o" "src\vm\ctrl\pointnclick.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\topdown.o" "src\vm\ctrl\topdown.c"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\shooting.o" "src\vm\ctrl\shooting.c"
  echo Ok.
  @echo,
) else (
  call :COLOR 3F "2. Ignore compiling the controllers..."
  echo Ok.
  @echo,
)

call :COLOR 3F "3. Compile the VM..."
@echo on
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm.o" "src\vm\vm.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_actor.o" "src\vm\vm_actor.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_actor_aux.o" "src\vm\vm_actor_aux.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_audio.o" "src\vm\vm_audio.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_device.o" "src\vm\vm_device.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_device_ext.o" "src\vm\vm_device_ext.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_effects.o" "src\vm\vm_effects.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_emote.o" "src\vm\vm_emote.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_game.o" "src\vm\vm_game.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_graphics.o" "src\vm\vm_graphics.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_gui.o" "src\vm\vm_gui.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_gui_label.o" "src\vm\vm_gui_label.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_gui_menu.o" "src\vm\vm_gui_menu.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_gui_progressbar.o" "src\vm\vm_gui_progressbar.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_input.o" "src\vm\vm_input.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_isr.o" "src\vm\vm_isr.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_memory.o" "src\vm\vm_memory.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_native.o" "src\vm\vm_native.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_object.o" "src\vm\vm_object.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_persistence.o" "src\vm\vm_persistence.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_physics.o" "src\vm\vm_physics.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_projectile.o" "src\vm\vm_projectile.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_scene.o" "src\vm\vm_scene.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_scene_transition.o" "src\vm\vm_scene_transition.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_scroll.o" "src\vm\vm_scroll.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_serial.o" "src\vm\vm_serial.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_system.o" "src\vm\vm_system.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_trigger.o" "src\vm\vm_trigger.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\vm_instructions.o" "src\vm\vm_instructions.c"
call %gbdk%\lcc.exe %cflag% -c -o "output\vm\%main%.o" "src\vm\%main%.c"
@echo off
echo Ok.
@echo,

if %rebuild%==1 (
  call :COLOR 3F "4. Compile the data..."
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\%bootstrap%.o" "src\vm\data\%bootstrap%.s"
  call %gbdk%\lcc.exe %cflag% -c -o "output\vm\%builtin%.o" "src\vm\data\%builtin%.c"
  echo Ok.
  @echo,
) else (
  call :COLOR 3F "4. Ignore compiling the data..."
  echo Ok.
  @echo,
)

call :COLOR 3F "5. Link..."
set libobj=^
  "output\vm\sgb_mouse.o" ^
  "output\vm\sfx_player.o" ^
  "output\vm\speech.o" ^
  "output\vm\exception.o" ^
  "output\vm\font.o" ^
  "output\vm\graphics.o" ^
  "output\vm\scroll.o" ^
  "output\vm\sgb.o" ^
  "output\vm\sleep.o" ^
  "output\vm\text.o" ^
  "output\vm\timer_handler.o" ^
  "output\vm\utils.o" ^
  "output\vm\utils_banked.o"
if %withcrashhandler%==1 (
  set libobj=%libobj% "output\vm\crash_handler.o"
)
set ctrlobj=^
  "output\vm\controller.o" ^
  "output\vm\navigation.o" ^
  "output\vm\platformer.o" ^
  "output\vm\pointnclick.o" ^
  "output\vm\topdown.o" ^
  "output\vm\shooting.o"
set dataobj="output\vm\%bootstrap%.o" ^
  "output\vm\%builtin%.o"
@echo on
call %gbdk%\lcc.exe %lflag% -o "output\vm\gbbvm.gb" ^
  %libobj% ^
  %ctrlobj% ^
  "output\vm\vm.o" ^
  "output\vm\vm_actor.o" ^
  "output\vm\vm_actor_aux.o" ^
  "output\vm\vm_audio.o" ^
  "output\vm\vm_device.o" ^
  "output\vm\vm_device_ext.o" ^
  "output\vm\vm_effects.o" ^
  "output\vm\vm_emote.o" ^
  "output\vm\vm_game.o" ^
  "output\vm\vm_graphics.o" ^
  "output\vm\vm_gui.o" ^
  "output\vm\vm_gui_label.o" ^
  "output\vm\vm_gui_menu.o" ^
  "output\vm\vm_gui_progressbar.o" ^
  "output\vm\vm_input.o" ^
  "output\vm\vm_isr.o" ^
  "output\vm\vm_memory.o" ^
  "output\vm\vm_native.o" ^
  "output\vm\vm_object.o" ^
  "output\vm\vm_persistence.o" ^
  "output\vm\vm_physics.o" ^
  "output\vm\vm_projectile.o" ^
  "output\vm\vm_scene.o" ^
  "output\vm\vm_scene_transition.o" ^
  "output\vm\vm_scroll.o" ^
  "output\vm\vm_serial.o" ^
  "output\vm\vm_system.o" ^
  "output\vm\vm_trigger.o" ^
  "output\vm\vm_instructions.o" ^
  "output\vm\%main%.o" ^
  %dataobj%
@echo off
echo Ok.
@echo,

REM Copy.
call :COLOR 2F "Copying..."
if exist "output\vm\gbbvm.gb" (
  xcopy /y /e "output\vm\gbbvm.gb" "gbbasic\kernels\*.gb"
  xcopy /y /e "output\vm\gbbvm.sym" "gbbasic\kernels\*.sym"
) else (
  echo gbbvm.gb doesn't exist.
)
echo Ok.
@echo,

REM Finish.
echo Building done!
goto :eof

REM Colorizer.
:COLOR
echo off
<nul set /p ".=%DEL%" > "%~2"
findstr /v /a:%1 /R "^$" "%~2" nul
del "%~2" > nul 2>&1
echo.
goto :eof

exit /b 0
