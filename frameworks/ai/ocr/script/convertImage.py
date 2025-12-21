#!/usr/bin/env python3
from time import time
from PIL import Image
import numpy as np
import cv2
import sys
import os
if __name__ == "__main__":
    # reading image
    path = ''
    outpath="./"
    if sys.argv[1]:
        if os.path.isdir(sys.argv[1]):
            dirs = os.listdir(sys.argv[1])
            path = sys.argv[1]
        else:
            dirs = {sys.argv[1]}
    else:
        print("** missing input image")
        exit(-1)

    for lfile in dirs:
        print(lfile)
        image = Image.open(os.path.join(path, lfile))
        image = image.convert('L')

        print("image size {} {}".format(image.size, image.mode))

        if 0:
            transformer = resizeNormalize((w, 32))
            image = transformer(image)
            image_c = image.view(1, *image.size())
            image_c = Variable(image_c)
        else:
            imgH = 32
            scale = image.size[1]*1.0 / imgH
            w     = image.size[0] / scale
            w     = int(w)

            if 0:
                # opencv resize
                image = cv2.resize(np.array(image), (w,imgH))
                image = Image.fromarray(image)
            else:
                # PIL resize
                image = image.resize((w,imgH),Image.BILINEAR)
            w,h   = image.size

            image_np = np.array(image)

        with open("{}/{}.dat".format(outpath, os.path.basename(lfile)), 'wb') as f:
            data = image_np
            data.tofile(f)
