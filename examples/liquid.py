
"""This examples demonstrates a simplish water effect of an
image. It attempts to create a hardware display surface that
can use pageflipping for faster updates. Note that the colormap
from the loaded GIF image is copied to the colormap for the
display surface.

This is based on the demo named F2KWarp by Brad Graham of Freedom2000
done in BlitzBasic. I was just translating the BlitzBasic code to
pygame to compare the results. I didn't bother porting the text and
sound stuff, that's an easy enough challenge for the reader :]"""

import math, os
import pygame, pygame.image
from pygame.locals import *



def main():
    #initialize and setup screen
    pygame.init()
    screen = pygame.display.set_mode((640, 480), HWSURFACE|DOUBLEBUF, 8)

    #load image
    imagename = os.path.join('data', 'liquid.gif')
    bitmap = pygame.image.load(imagename)

    #get the image and screen in the same format
    if screen.get_bitsize() == 8:
        screen.set_palette(bitmap.get_palette())

    #prep some variables
    anim = 0.0
    sin = math.sin  #note, making sin local speeds up call (slightly)
    xstep, ystep = range(0, 640, 20), range(0, 480, 20)
    stopevents = QUIT, KEYDOWN, MOUSEBUTTONDOWN

    #mainloop
    while not pygame.event.peek(stopevents):
        anim += 0.4
        for x in xstep:
            for y in ystep:
                xpos = (x+(sin(anim+x*.01)*15))+20
                ypos = (y+(sin(anim+y*.01)*15))+20
                screen.blit(bitmap, (x, y), (xpos, ypos, 20, 20))
        pygame.display.flip()

if __name__ == '__main__':
    main()



"""BTW, here is the code from the BlitzBasic example this was derived
from. This code runs a little faster, yet reads a lot slower. again i've
snipped the sound and text stuff out.
-----------------------------------------------------------------
; Brad@freedom2000.com

; Load a bmp pic (800x600) and slice it into 1600 squares
Graphics 640,480					
SetBuffer BackBuffer()				
bitmap$="f2kwarp.bmp"					
pic=LoadAnimImage(bitmap$,20,15,0,1600)

; use SIN to move all 1600 squares around to give liquid effect
Repeat
f=0:w=w+10:If w=360 Then w=0
For y=0 To 599 Step 15
For x = 0 To 799 Step 20
f=f+1:If f=1600 Then f=0
DrawBlock pic,(x+(Sin(w+x)*40))/1.7+80,(y+(Sin(w+y)*40))/1.7+60,f 
Next:Next:Flip:Cls
Until KeyDown(1)
"""
