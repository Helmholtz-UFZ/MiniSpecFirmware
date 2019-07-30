README for developers
=====================


Releases
-----------
For a release, 
* commit all changes
* make clean - improtant for building the correct version string inside the firmware (comand `version`)
* build project
* check the date of the **.bin**
* put img on the device
* check `version` command (no dirty should appear)
* rename **.bin** - should contain the full tag and hash -> run `git describe --always --long --dirty`
* release it, fly lill' bird, fly..


