# tf-convex-polygon-iou

Convex Polygons Intersection over Union (IoU) computation for tensorflow.

This repository is a a generalization of https://github.com/lilanxiao/Rotated_IoU to convex polygons, and ported to tensorflow 2.


## Approach

The algorithm here may not be optimal, but only uses available tensorflow operations.

The most complex parts are to compute the intersection of two convex polygon.

As for the original work, it is based on [Livermore, Califf, 1977](https://www.osti.gov/servlets/purl/7309916/). which remarks:

* Intersection of two convex polygons is a convex polygon
* A vertex from a polygon that is contained in the other polygon is a vertex of the intersection shape. 
* An edge from a polygon that is contained in the other polygon is an edge in the intersection shape. 
* Edge intersections between two polygons are vertices in the intersection shape.

Therefore the algorithm here:
1. Finds all vertices in a polygon that lies within the other, by computing the [winding number](https://web.archive.org/web/20130126163405/http://geomalgorithms.com/a03-_inclusion.html).
2. Finds all intersection between edge other edges of the two polygons.
3. Sorts all vertices in trigonometric order arround a point inside the polygon (die to duplicates, it cannot be ensured that it is the centroid of the polygon).

To compute the IoU, one must finally compute the area of the intersection. The used formula is robust to duplicate vertices, if the order of vertices is right.

## Requirements

```
tensorflow >= 2.8.0 # it may be lower, but it is not tested
```

