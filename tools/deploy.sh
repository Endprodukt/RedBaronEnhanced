#!/bin/sh
# deploy.sh - copy the Pi build and the package files to the cabinet. Only
# copies: what runs there is not touched (a running Red Baron keeps the old
# program until it is started again from the menu). Run from the project's
# folder, in Git Bash, after the Pi build (BUILD.md).
#
#   tools/deploy.sh             copy
#   tools/deploy.sh --restart   copy, end a running Red Baron and start the
#                               new one (the menu is stopped meanwhile and
#                               comes back when the game is left:
#                               package/run.sh); only when the owner asks
#                               for it. Further arguments go to the game.
K="-i $HOME/.ssh/id_ed25519_vectorpie -o ConnectTimeout=8 -o BatchMode=yes"
H=pi@192.168.0.122
D=/persistent/games/rbenhanced
RESTART=0
if [ "$1" = "--restart" ]; then RESTART=1; shift; fi
ssh $K $H "mkdir -p $D/save" || exit 1
scp -q $K build/pi/redbaron $H:$D/redbaron.new || exit 1
scp -q $K package/game.ini package/redbaron.ini package/run.sh package/marquee.png $H:$D/ || exit 1
ssh $K $H "mkdir -p $D/sounds" || exit 1
scp -q $K package/sounds/*.ogg package/sounds/CREDITS.txt $H:$D/sounds/ || exit 1
# (the program replaced by a rename: a running one keeps its own file)
ssh $K $H "cd $D && sed -i 's/\r\$//' game.ini redbaron.ini run.sh && chmod 755 redbaron.new run.sh && mv -f redbaron.new redbaron && md5sum redbaron && grep '^version' game.ini && (pgrep -x redbaron > /dev/null && echo 'Red Baron is running (the old program until it is started again)' || echo 'Red Baron is not running')" || exit 1
if [ $RESTART = 1 ]; then
    ssh $K $H "cd $D && pkill -INT -x redbaron; for i in 1 2 3 4 5 6; do pgrep -x redbaron > /dev/null || break; sleep 0.5; done; pkill -KILL -x redbaron; (setsid nohup ./run.sh $* > /dev/null 2>&1 &) ; sleep 6; pgrep -x redbaron > /dev/null && echo game-running || echo GAME-NOT-RUNNING; tail -n 5 run.log"
fi
md5sum build/pi/redbaron
