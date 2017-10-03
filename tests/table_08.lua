x = {}
x.a = {}
x.a.b = {}
x.a.b.c = 5
print(x.a.b.c)
y = x
y.a.b.c = 10
print(x.a.b.c)
