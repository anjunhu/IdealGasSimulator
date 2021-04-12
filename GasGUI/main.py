from tkinter import *
import random
import serial
import numpy as np

WIDTH = 500
HEIGHT = 500
GRANULARITY = 10
IDEAL_GAS_CONST = 8.31446261815324

class Ball:

    def __init__(self):
        self.xpos = random.randint(0, WIDTH)
        self.ypos = random.randint(0, HEIGHT)
        self.vx = random.random() - 0.5
        self.vy = random.random() - 0.5


class MyCanvas(Canvas):

    def __init__(self, master):

        super().__init__(master, width=WIDTH , height=HEIGHT, bg="grey", bd=0, highlightthickness=0, relief="ridge")
        self.label = Label(master, text="MESSAGE")
        self.label.pack(side=BOTTOM)
        self.balls = []   # keeps track of Ball objects
        self.bs = []      # keeps track of Ball objects representation on the Canvas
        self.work = 0
        self.temp = 25
        self.pres = 1013
        for _ in range(20):
            ball = Ball()
            self.balls.append(ball)
            self.bs.append(self.create_oval(ball.xpos - 3, ball.ypos - 3, ball.xpos + 3, ball.ypos + 3, fill="white"))
        self.run()

    def run(self):
        try:
            self.temp, self.work, self.pres = read_sensors()
            info = "T = {} K   P = {} mbar   Internal Energy = {:.2f} W   " \
                   "External Work = {} mW".format(self.temp + 273, self.pres,
                                                IDEAL_GAS_CONST * 1.5 * self.temp,
                                                int(self.work))
            self.label.config(text=info)
        except ValueError:
            pass

        for b, ball in zip(self.bs, self.balls):
            # print(self.temp, self.work)
            self.move(b, ball.vx, ball.vy)
            pos = self.coords(b)
            if pos[3] >= HEIGHT or pos[1] <= 0:
                ball.vy = - ball.vy
            if pos[2] >= WIDTH or pos[0] <= 0:
                ball.vx = - ball.vx
            speed = (self.temp - 25) + self.work + random.random()
            speed = abs(speed * 0.05)
            ball.vy = np.sign(ball.vy) * speed
            ball.vx = np.sign(ball.vx) * speed

        self.after(7, self.run)


def read_sensors():
    line = ser.read(200).decode()
    # AngAc = line[2+line.index("=", line.find('AngAc')):line.index("]", line.find('AngAc'))].split(", ")
    temp = int(line[1 + line.index("=", line.find('T')):line.index("\n", line.find('T'))])
    work = int(line[1 + line.index("=", line.find('W')):line.index("\n", line.find('W'))])
    pres = int(line[1 + line.index("=", line.find('P')):line.index("\n", line.find('P'))])
    return temp, work, pres




if __name__ == '__main__':
    ser = serial.Serial('COM3', 115200, timeout=0)

    window = Tk()
    window.geometry("{}x{}".format(WIDTH, HEIGHT))
    window.title("Gassy Box")
    c = MyCanvas(window)
    c.pack(expand=YES, fill=BOTH)

    window.mainloop()