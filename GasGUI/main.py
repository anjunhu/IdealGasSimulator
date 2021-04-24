from tkinter import *
import random
import serial
import numpy as np

WIDTH = 500
HEIGHT = 500
IDEAL_GAS_CONST = 8.31446261815324
ATM_PRESSURE = 1013

# Reference: https://stackoverflow.com/questions/49710067/generating-multiple-moving-objects-in-the-tkinter-canvas
class Particle:

    def __init__(self):
        self.xpos = random.randint(0, WIDTH)
        self.ypos = random.randint(0, HEIGHT)
        self.vx = random.random() - 0.5
        self.vy = random.random() - 0.5


class MyCanvas(Canvas):

    def __init__(self, master):

        super().__init__(master, width=WIDTH, height=HEIGHT, bg="grey", bd=0, highlightthickness=0, relief="ridge")
        self.label = Label(master, text="MESSAGE")
        self.label.pack(side=BOTTOM)
        self.particles = []  # keeps track of Particle objects
        self.pstate = []  # keeps track of Particle objects representation on the Canvas
        self.work = 0  # physical quantities of the system
        self.temp = 25
        self.pres = 1013
        for _ in range(20): # create 20 particles, randomly placed on the canvas
            particle = Particle()
            self.particles.append(particle)
            self.pstate.append(
                self.create_oval(particle.xpos - 3, particle.ypos - 3, particle.xpos + 3, particle.ypos + 3,
                                 fill="white"))
        self.run()

    def run(self):
        # Read from UART and update the system's physical quantities
        try:
            self.temp, self.work, self.pres = read_sensors()
            info = "T = {} K   P = {} mbar   Internal Energy = {:.2f} W   " \
                   "External Work = {} mW".format(self.temp + 273, self.pres,
                                                  IDEAL_GAS_CONST * 1.5 * self.temp,
                                                  int(self.work))
            self.label.config(text=info)
        except ValueError:
            pass

        # Move the particles in accordance to measurements
        for b, particle in zip(self.pstate, self.particles):
            # print(self.temp, self.work)
            self.move(b, particle.vx, particle.vy)
            pos = self.coords(b)
            if pos[3] >= HEIGHT or pos[1] <= 0:
                particle.vy = - particle.vy
            if pos[2] >= WIDTH or pos[0] <= 0:
                particle.vx = - particle.vx
            speed = (self.temp - 25)*self.pres/ATM_PRESSURE + self.work + random.random()
            speed = abs(speed * 0.05)
            particle.vy = np.sign(particle.vy) * speed
            particle.vx = np.sign(particle.vx) * speed

        self.after(10, self.run)  # Sampling rate = 100 Hz


def read_sensors():
    line = ser.read(200).decode()  # Read 200 bytes and convert to ASCII strings
    # AngAc = line[2+line.index("=", line.find('AngAc')):line.index("]", line.find('AngAc'))].split(", ")
    temp = int(line[1 + line.index("=", line.find('T')):line.index("\n", line.find('T'))])
    work = int(line[1 + line.index("=", line.find('W')):line.index("\n", line.find('W'))])
    pres = int(line[1 + line.index("=", line.find('P')):line.index("\n", line.find('P'))])
    return temp, work, pres


if __name__ == '__main__':
    ser = serial.Serial('COM3', 115200, timeout=0)

    window = Tk()
    window.geometry("{}x{}".format(WIDTH, HEIGHT))
    window.title("Gassy Box V=1m^3")
    c = MyCanvas(window)
    c.pack(expand=YES, fill=BOTH)

    window.mainloop()
