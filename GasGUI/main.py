from tkinter import *
import random
import serial

WIDTH = 500
HEIGHT = 250
GRANULARITY = 10

class Ball:

    def __init__(self):
        self.xpos = random.randint(0, 254)
        self.ypos = random.randint(0, 310)
        self.xspeed = 0.1
        self.yspeed = 0.1


class MyCanvas(Canvas):

    def __init__(self, master):

        super().__init__(master, width=WIDTH , height=HEIGHT, bg="grey", bd=0, highlightthickness=0, relief="ridge")
        self.pack()

        self.balls = []   # keeps track of Ball objects
        self.bs = []      # keeps track of Ball objects representation on the Canvas
        for _ in range(20):
            ball = Ball()
            self.balls.append(ball)
            self.bs.append(self.create_oval(ball.xpos - 3, ball.ypos - 3, ball.xpos + 3, ball.ypos + 3, fill="white"))
        self.run()

    def run(self):

        for b, ball in zip(self.bs, self.balls):
            try:
                temp = read_sensors()
                ball.yspeed = max(0, (temp-25)*0.5)
                ball.xspeed = max(0, (temp-25)*0.5)
                print("ball {} T {} speed {}".format(ball, temp, ball.yspeed))
            except ValueError:
                pass
            self.move(b, ball.xspeed, ball.yspeed)
            pos = self.coords(b)
            if pos[3] >= HEIGHT or pos[1] <= 0:
                ball.yspeed = - ball.yspeed
            if pos[2] >= WIDTH or pos[0] <= 0:
                ball.xspeed = - ball.xspeed
        self.after(10, self.run)


def read_sensors():
    line = ser.read(100).decode()
    return int(line[1+line.index("=", line.find('T')):line.index("\n", line.find('T'))])


if __name__ == '__main__':
    ser = serial.Serial('COM3', 115200, timeout=0)

    window = Tk()
    window.geometry("{}x{}".format(WIDTH, HEIGHT))
    window.title("Gassy Box")
    c = MyCanvas(window)
    c.pack(expand=YES, fill=BOTH)

    message = Label(window, text="Gassy Box")
    message.pack(side=BOTTOM)

    # while True:
        # update_speed()
        # window.update()

    window.mainloop()
#
# if __name__ == '__main__':
#     master = Tk()
#     master.title("Gassy Box")
#     w = Canvas(master,
#                width=WIDTH,
#                height=HEIGHT, bg="grey")
#     w.pack(expand=YES, fill=BOTH)
#     w.bind("<B1-Motion>", paint)
#
#     message = Label(master, text="Gassy Box")
#     message.pack(side=BOTTOM)
#     particles = []
#     for i in range(50):
#         particles.append(Particle("particle{}".format(i)))
#     mainloop()
