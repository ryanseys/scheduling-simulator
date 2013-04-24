# Scheduling Simulator

### A process scheduling simulator supporting FCFS, SJF, SRTF, and Round Robin policies.

Example Format for input files for simulation:
```
1,0,22,5,1,2
3,12,12,5,1,2
5,17,14,5,1,2
2,9,11,5,1,2
4,13,11,5,1,2
```
column values (in order):

pid, start time, total cpu time, io freqency, io duration, round robin time slice freqency

Run using the Makefile provided. You need to have the glib-2.0 library installed on your system for it to compile correctly.

### Authors: Ryan Seys and Osazuwa Omigie
