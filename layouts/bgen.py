import math
import json

class _Pixel(object):
    def __init__(self, x, y, z=0.0000):
        self.__x = x
        self.__y = y
        self.__z = z

    def __str__(self):
        rs = '{{"point": [{0:0.4f}, {1:0.4f}, {2:0.4f}]}}'.format(self.__x, self.__y, self.__z)
        return rs
    
class LayoutGenerator(object):
    def __init__(self):
        self.__descriptions = {}
        self.__pixels = []
        self.__max_x = -1
        self.__max_y = -1
        self.__scale = 0.01

    def __points_on_a_circle(self, radius, count):
        pi = 3.14
        scale = self.__scale
        for x in xrange(0, count):
            y = math.cos(2*pi/count*x)*radius
            x = math.sin(2*pi/count*x)*radius
            x = x * scale
            y = y * scale
            if x > self.__max_x:
                self.__max_x = x
            if y > self.__max_y:
                self.__max_y = y

            yield (x, y)
        
    def add_concentric(self, name, cx=0, cy=0):
        assert name not in self.__descriptions, \
            "duplicate name {}".format(name)

        rings = [ (32, 111.8), (24, 91.8), (16, 71.8), (12, 51.8), (8, 31.8) ]
        ring_counts = []
        cnt = 0
        start = len(self.__pixels)
        for ring_cnt, ring_diag in rings:
            ring_rad = ring_diag / 2
            ring_counts.append(ring_cnt)
            for x, y in self.__points_on_a_circle(ring_rad, ring_cnt):
                cnt += 1
                self.__pixels.append(_Pixel(x + cx, y + cy))

        self.__pixels.append(_Pixel(0,0))
        cnt += 1
        ring_counts.append(1)
        self.__descriptions[name] = { 
            "type": "conring", "start": start, "length": cnt,
            "ring_counts": ring_counts }

    @property
    def max_x(self):
        return self.__max_x
    @property
    def max_y(self):
        return self.__max_y
    
    def add_banner(self, name, width = 32, height = 8, ulx = 0, uly = 0):
        assert name not in self.__descriptions, \
            "duplicate name {}".format(name)

        self.__descriptions[name] = { "type": "banner", "width": width, "height": height,
                                      "start": len(self.__pixels)}
        self.__do_banner_pixels(width, height, ulx, uly)

    def __do_banner_pixels(self, width, height, ulx, uly):
        distance_mm = 10
        ulx = ulx - ((width / 2) * distance_mm)
        px = ulx
        py = uly
        for y in range(0, height):
            for x in range(0, width):
                self.__pixels.append(_Pixel(px * self.__scale, py * self.__scale))
                px += distance_mm
            px = ulx
            py += distance_mm

    def add_strip(self, name, width, ulx = 0, uly = 0):
        assert name not in self.__descriptions, \
            "duplicate name {}".format(name)

        self.__descriptions[name] = { "type": "strip", "width": width, "start": len(self.__pixels)}
        self.__do_banner_pixels(width, 1, ulx, uly)
        
    def printit(self):
        print "["
        print '    {{"displays": {} }},'.format(json.dumps(self.__descriptions, indent=4))
        print "   ",
        sl = []
        for pix in self.__pixels:
            sl.append(str(pix))

        print ",\n    ".join(sl)
        print "]"


ly = LayoutGenerator()
ly.add_banner("b1", uly=100)
ly.add_strip("s1", 120, 1, uly=200)
ly.add_strip("s2", 120, 1, uly=220)
ly.add_concentric("c1")
ly.printit()
