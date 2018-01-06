#!/bin/sh

MODEL_FILE="/home/jerry/projects/saba/build/resource/mmd/maya/maya ver1.04/maya1.pmx"
ANIM_FILE=/home/jerry/projects/saba/build/../../MMDLoader/data/miku.vmd
FRAME=0
ANIM_TIME=0
./bin/main -p "$MODEL_FILE" -v $ANIM_FILE -f $FRAME -t $ANIM_TIME
