#!/usr/bin/env python
# -*- coding: utf-8 -*-

D = 1
I = 2
N = 3
W = 4
E = 5


def mLog(flag, x):
    "This Module is a Dbg Lib"
    if flag == D:
        print "\033[34;1m", x, "\033[0m"  # Blue
    elif flag == I:
        print "\033[33;1m", x, "\033[0m"  # Yellow
    elif flag == N:
        print "\033[32;1m", x, "\033[0m"  # Green
    elif flag == W:
        print "\033[35;1m", x, "\033[0m"  # Purple
    elif flag == E:
        print "\033[31;1m" + "[ERR]", x, "\033[0m"  # Red


if __name__ == '__main__':
    mLog(D, "This is mDbg Test")
    mLog(I, "This is mDbg Test")
    mLog(N, "This is mDbg Test")
    mLog(W, "This is mDbg Test")
    mLog(E, "This is mDbg Test")


