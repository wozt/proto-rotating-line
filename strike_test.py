import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

x = np.linspace(-3, 3, 1000)
fig, ax = plt.subplots()
line, = ax.plot(x, np.zeros_like(x), color='gold', lw=2)
ax.set_ylim(-1.5, 1.5)

def update(frame):
    t = frame / 10
    # Enveloppe qui grandit dans le temps
    amplitude = 1 / (1 + np.exp(-4 * (t - 2)))
    # Fractale de Weierstrass
    weierstrass = sum((0.5)**n * np.cos(3**n * x) for n in range(1, 6))
    line.set_ydata(amplitude * weierstrass)
    return line,

ani = FuncAnimation(fig, update, frames=40, interval=50, blit=True)
plt.show()
