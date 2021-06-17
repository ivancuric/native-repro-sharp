#!/bin/bash
sysctl -w kern.sysv.shmmax=4194304
sysctl -w kern.sysv.shmall=1024
