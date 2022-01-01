#!/usr/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir="/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/python"
export GR_CONF_CONTROLPORT_ON=False
export PATH="/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/build/python":$PATH
export LD_LIBRARY_PATH="":$LD_LIBRARY_PATH
export PYTHONPATH=/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/build/swig:$PYTHONPATH
/usr/bin/python3 /mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/python/qa_wifi_samples_align.py 
