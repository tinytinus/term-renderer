# terminal renderer

A simple C and nCurses terminal shape-renderer

Usage :
```
./term_renderer [-f filename.shape | filename.csv]
```

## TODO:

 - [x] color support
 - [x] automatic scaling
 - [x] the right symbol
 - [x] CSV rendering system
 - [x] custom more readable .shape
 - [x] correct drawing order

## shapes
the shapes consist of 3 parts: the points, the edges (aka. lines) and the colours.

### points 

The base of the entire thing works around the points, they have coordinates in the X, Y and Z axis and are declared first in the CSV file.
This project started with just a few spinning points and they are still here.
Because they are so simple there is not a lot more to say.

### edges 

 The line drawing happens using Bresenham's line algorithm.
 In short i draw a line between two points and the check for each 'tile' if it lies on that line 
```
    y1 - y0
y = _______ (x - x0) + y0
    x1 - x0
```
i took this math from Wikipedia, for a real explanation see [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham's_line_algorithm).

### color

 Color support was one of the last things i made but also one of the most noticable things.
 Its quite easy to do using the nCurses library and required only a few changes in the end.

 There is support for 7 colors:
  1. white
  2. red
  3. cyan
  4. yellow
  5. green
  6. blue
  7. magenta

## spinning 

 Another core part of the project is the spinning of the shapes, this required a bit of math 

 First we need to know the shape to spin and the angle to spin at, this was simple.
 
 Secondly we take the sinus and the cosinus of the angle:
```
float sin = sinf(angle);
float cos = cosf(angle);
```

 Finnaly the points get turned using:
```
.X = X * cos + Z * sin
.Y = y
.Z = -X * sin + Z * cos
```

 The reason why Y stays the same is personal prefrence, i decided that it would look good if the shapes spun around their Y axis.

## .shape

 originally i used CSV for this project but as i moved on CSV became less and less readable, this is why i made my own custom filetype .shape
 .shape has really easy syntax
```
# Comments start with #
# vertex <name> <x>,<y>,<z>
# edge <vertex1>-<vertex2> [color=<num>]

vertex v0  0.0, 0.3, 0.5
vertex v1  0.5,-0.3, 0.0
vertex v2 -0.5,-0.3, 0.0
vertex v3  0.0,-0.3, 1.0

edge v0-v1 color=2
edge v1-v2 color=3
edge v2-v0 color=4
edge v3-v0 color=5
edge v3-v1 color=6
edge v3-v2 color=7

```
 you have 2 commands:
	- vertex, creates a vertex(point) with a name at the location x, y, z
	- edge, creates a edge between 2 points (by name) with a color

## CSV

 CSV stands for "comma separated values" and is used to store data in collunms like this 
```
first_name,second_name,age
alice,jones,27
bob,smith,30
```
or like this
```
NAME
alice,jones
bob,smith

AGE
27
30
```

### how i used CSV's
 
 First we have a part named POINTS:
```
POINTS
0.0,0.3,0.5
0.5,-0.3,0.0
-0.5,-0.3,0.0
0.0,-0.3,1.0

```
 In points we store the points and their X, Y and Z position.
 Next we have EDGES:
```
EDGES
0,1,2
1,2,3
2,0,4

3,0,5
3,1,6
3,2,7
```
 This stores which lines need to be drawn using: the origin, the endpoint and the color.

> [NOTE]  
> I put in a max limit of 100 points and 200 lines, if you want you can change this in the code if you want.

### some fun shapes's

 Rainbow piramid:
```
# Rainbow Piramid
vertex v0  0.0, 0.3, 0.5
vertex v1  0.5,-0.3, 0.0
vertex v2 -0.5,-0.3, 0.0
vertex v3  0.0,-0.3, 1.0

edge v0-v1 color=2
edge v1-v2 color=3
edge v2-v0 color=4
edge v3-v0 color=5
edge v3-v1 color=6
edge v3-v2 color=7

```


## reflection

 This was a really fun project to do i learned about nCurses and made something i genuinely like looking at !
 I hope that you also enjoy it and have a nice day. 
 
