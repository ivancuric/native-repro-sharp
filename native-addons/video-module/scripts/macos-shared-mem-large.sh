#!/bin/bash
sysctl -w kern.sysv.shmmax=16777216
sysctl -w kern.sysv.shmall=16384
