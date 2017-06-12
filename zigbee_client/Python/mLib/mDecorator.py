#!/usr/bin/env python
# -*- coding: utf-8 -*-

"This Module is a Decorator Lib"


# 装饰器，可以用来捕获按键的ctrl c命令，然后调用回调函数callback来处理异常，如果callback没有设置，则不会做任何动作
def catch_keyboard_interrupt(callback=None):
    def decorator(fn):
        def wrapper(*args):
            try:
                return fn(*args)
            except KeyboardInterrupt:
                print 'exit process by key interrupt'
                if callback:
                    callback()
                exit(0)
        return wrapper
    return decorator


@catch_keyboard_interrupt
def main():
    print "This is Test"
    while True:
        pass

if __name__ == '__main__':
    main()



