# Dahlquist

The model implements the [Dahlquist](https://en.wikipedia.org/wiki/Germund_Dahlquist) test equation.

```
der(x) = -k * x
```

whith

| Variable      | Description    | Start |
|:--------------|:---------------| -----:|
| x             | The only state |     1 |
| der(x)        | The derivative |     0 |
| k             | Parameter      |     1 |

The analytical solution of this system is

```
x(t) = exp(-k * t)
```

The plot shows the [reference result](Dahlquist_ref.csv) computed with [FMPy](https://github.com/CATIA-Systems/FMPy).

![plot](Dahlquist_ref.svg)
