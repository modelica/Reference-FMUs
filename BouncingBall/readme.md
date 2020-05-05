# BouncingBall

The BouncingBall implements the following equation:

```
der(h) =  v
der(v) = -g

when h <= 0    then h := 0 and v := -e * v
when v < v_min then h := 0 and v := 0
```

whith the variables

| Variable | Start | Unit | Causality | Variability | Description
|:---------| -----:|:-----|-----------|-------------|:---------------
| h        |     1 | m    | output    | continuous  | Position of the ball
| v        |     0 | m/s  | output    | continuous  | Velocity of the ball
| g        |  9.81 | m/s2 | parameter | fixed       | Gravity acting on the ball
| e        |   0.7 |      | parameter | tunable     | Coefficient of restitution
| v_min    |   0.1 | m/s2 | parameter | constant    | Velocity below which the ball stops bouncing

The plot shows the [reference result](BouncingBall_ref.csv) computed with [FMPy](https://github.com/CATIA-Systems/FMPy).

![plot](BouncingBall_ref.svg)
