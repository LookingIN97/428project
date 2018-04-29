# 428project
Author: Wenzhang Qian (wenzhang@ualberta.ca)

## Dependencies
Opencv2 (2.4 was tested)

## Compile and run
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


## Dataset
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
The dataset I use is Princeton Tracking Benchmark from http://tracking.cs.princeton.edu.
The filenames are modified by temp.py to make it easier for further processing.