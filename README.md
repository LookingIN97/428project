# 428project
Author: Wenzhang Qian (wenzhang@ualberta.ca)

## Dependencies
Opencv2 (2.4 was tested)

## compile and run
compile
```commandline
mkdir build
mkdir bin
cd build
cmake ../
make
```

run
```commandline
cd bin
./main <path_to_dataset>
```

The structure of a dataset is assumed to be
```
.
+-- init.text (initial bounding box)
+-- rgb
|   +-- 1.png
|   +-- 2.png
|   +-- n.png
+-- depth
|   +-- 1.png
|   +-- 2.png
|   +-- n.png
```
