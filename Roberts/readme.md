# Robertson Problem

A problem from chemical kinetics, due to Robertson, that consists of the three equations

```
dy1/dt = -0.04 * y1 + 1e4 * y2 * y3
dy2/dt =  0.04 * y1 - 1e4 * y2 * y3 - 3e7 * y2 * y2
     0 = y1 + y2 + y3 - 1
```

with initial conditions

```
y1 = 1
y2 = y3 = 0
```

and roots at

```
y1 = 1e-4
y3 = 0.01
```
