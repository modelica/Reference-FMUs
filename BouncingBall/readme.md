# BouncingBall

The BouncingBall implements the following system of equations:

```
der(h) = v
der(v) = g

when h <= 0    then h := 0 and v := -e * v
when v < v_min then h := 0 and v := 0
```
