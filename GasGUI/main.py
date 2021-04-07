from tkinter import *
import random
import serial
import numpy as np

WIDTH = 500
HEIGHT = 250
GRANULARITY = 10

class Ball:

    def __init__(self):
        self.xpos = random.randint(0, WIDTH)
        self.ypos = random.randint(0, HEIGHT)
        self.xspeed = random.random() - 0.5
        self.yspeed = random.random() - 0.5


class MyCanvas(Canvas):

    def __init__(self, master):

        super().__init__(master, width=WIDTH , height=HEIGHT, bg="grey", bd=0, highlightthickness=0, relief="ridge")
        self.label = Label(master, text="MESSAGE")
        self.label.pack(side=BOTTOM)
        self.balls = []   # keeps track of Ball objects
        self.bs = []      # keeps track of Ball objects representation on the Canvas
        self.work = 0
        self.temp = 25
        for _ in range(20):
            ball = Ball()
            self.balls.append(ball)
            self.bs.append(self.create_oval(ball.xpos - 3, ball.ypos - 3, ball.xpos + 3, ball.ypos + 3, fill="white"))
        self.run()

    def run(self):
        try:
            self.temp, self.work = read_sensors()
            info = "T = {} K   Internal Energy = {:.2f} W   External Work = {:.2f}W".format(self.temp + 273,
                                                                                            8.31446261815324 * 1.5 * self.temp,
                                                                                            self.work)
            self.label.config(text=info)
        except ValueError:
            pass

        for b, ball in zip(self.bs, self.balls):
            # print(self.temp, self.work)
            self.move(b, ball.xspeed, ball.yspeed)
            pos = self.coords(b)
            if pos[3] >= HEIGHT or pos[1] <= 0:
                ball.yspeed = - ball.yspeed
            if pos[2] >= WIDTH or pos[0] <= 0:
                ball.xspeed = - ball.xspeed
            ball.yspeed = np.sign(ball.yspeed) * max(0, (self.temp - 25) ** 2 * 0.01 + self.work * 0.05)
            ball.xspeed = np.sign(ball.xspeed) * max(0, (self.temp - 25) ** 2 * 0.01 + self.work * 0.05)

        self.after(7, self.run)


def read_sensors():
    line = ser.read(200).decode()
    temp = int(line[1+line.index("=", line.find('T')):line.index("\n", line.find('T'))])
    AngAc = line[2+line.index("=", line.find('AngAc')):line.index("]", line.find('AngAc'))].split(", ")
    work = (abs(int(AngAc[0])) + abs(int(AngAc[1])) + abs(int(AngAc[2])))/2/3.14/1000
    return temp, work




if __name__ == '__main__':
    ser = serial.Serial('COM3', 115200, timeout=0)

    window = Tk()
    window.geometry("{}x{}".format(WIDTH, HEIGHT))
    window.title("Gassy Box")
    c = MyCanvas(window)
    c.pack(expand=YES, fill=BOTH)

    window.mainloop()