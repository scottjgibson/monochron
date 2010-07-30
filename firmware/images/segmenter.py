import Image
import sys

im = Image.open(sys.argv[1])
print "//Downloaded image: "+im.format, im.size, im.mode
if (im.format != "BMP") or (im.mode != "1"):
    exit

height = im.size[1]
width = im.size[0]
pix = im.load()
k = 0


print "const uint8_t %s[] PROGMEM = {" % (sys.argv[2])
for j in range(height):
    currentcolor = 255
    line = []
    startseg = 0
    endseg = 0
    for i in range(width):
       # print pix[i,j]
        #print "[",i,", ",j,"] -> ", i +width*(j/8)
        if (pix[i, j] != currentcolor):
            if (currentcolor == 255):
                # start of segment
                startseg = i
            else:
                # end of segment
                line.append([startseg, i-1])
            currentcolor = pix[i,j]
    if (currentcolor != 255):
        line.append([startseg, i])
    """if (pix[width-1,j] != 255):
        line.append([startseg, width-1])
    if (len(line)< 1):
        line.append([255,255])
    if (len(line)< 2):
        line.append([255,255])
        
    print "%d, %d, %d, %d," % (line[0][0], line[0][1],line[1][0],line[1][1])
    #print int2bin(line[0][0],8) + int2bin(line[0][1],8) + int2bin(line[1][0],8) + int2bin(line[1][1],8)"""
    print "%d," % (len(line)),
    k = k + (len(line) * 2) + 1
    for i in range(len(line)):
    	print "%d, %d," % (line[i][0], line[i][1]),
    print ""

print "//%s is %d bytes compresed, %d bytes uncompressed, saving %d bytes" % (sys.argv[1], k, ((height * width) / 8), ((height * width) / 8) - k)
print "};"
print ""

