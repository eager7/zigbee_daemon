#!/usr/bin/env python
# -*- coding: utf-8 -*-

from multiprocessing import Process,Queue
import signal,time

__metaclass__ = type


class mProcess():
    '''
    Process
    '''
    def __init__(self, pro_info=None, name=None):
        self.pro_info = pro_info
        self.name = name
        self.queue = Queue()
        self.process = Process(name=name, target=self.run, args=(self.queue,))

    def run(self, queue):
        while True:
            print self.name
            time.sleep(1)
            try:
                if not queue.get(block=False):
                    break
            except:
                pass

    def start(self):
        print 'start process %s' % self.name
        self.process.start()

    def stop(self):
        print 'stop process %s' % self.name
        self.queue.put(False)
        self.process.join()
        print 'child process %s exit success' % self.name

if __name__ == '__main__':
    print "This is mProcess Test"
    p = mProcess(name='test')
    p.start()
    time.sleep(5)
    p.stop()
