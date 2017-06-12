#!/usr/bin/env python
# -*- coding: utf-8 -*-

import threading
import signal
import time
from mDecorator import catch_keyboard_interrupt

__metaclass__ = type


class mThread(threading.Thread):
    '''This Module is a Thread Lib, New Class need overwrite run method'''
    def __init__(self, thread_info=None, name=None):
        super(mThread, self).__init__()
        self.thread_state = True
        self.thread_info = thread_info
        self.name = name

    def run(self):
        while self.thread_state:
            print "%s Run..." % self.name
            time.sleep(1)

    def stop(self):
        print "stop thread:", self.name
        self.thread_state = False
        signal.alarm(1)  # break select method

    def display(self):
        print "thread:", self.name, "is running"


@catch_keyboard_interrupt
def main():
    print "This is mThread Test"
    t = mThread(None, "Test")
    t.start()
    t.display()
    while True:
        print 'main loop'
        time.sleep(1)

if __name__ == '__main__':
    main()
