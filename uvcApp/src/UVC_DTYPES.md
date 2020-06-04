# UVC Data Types


DATA TYPE       |       Explanation
----------------|------------------------------------
UVC_FRAME_FORMAT_YUYV | YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and one UV (chrominance) pair for every two pixels. Can be converted to RGB or BGR
UVC_FRAME_FORMAT_RGB    | Standard RGB image.
UVC_FRAME_FORMAT_MJPEG  | Motion Jpeg compressed image. can be converted to RGB

**NOTE** BGR images cannot be grabbed directly from UVC cameras, but YUYV type images can be converted to this format. It is also the standard format for the OpenCV open computer vision library