package test

locale int x = 0
local int y = 43


func process<T>(T a, T b):
	return a * a + b

func main():
	print(process(x, y))
