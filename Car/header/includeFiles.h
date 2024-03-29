/*
 * includeFiles.h
 *
 *  Created on: 19 Feb 2013
 *      Author: hassan
 */

#ifndef INCLUDEFILES_H_
#define INCLUDEFILES_H_

#include <opencv2/highgui/highgui.hpp>
#include <dirent.h>
#include <cvblobs/blob.h>
#include <cvblobs/BlobResult.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <omp.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>  /* POSIX terminal control definitions */
#include <errno.h>    /* Error number definitions */
#include <sys/ioctl.h>
#include <getopt.h>
#include <iostream>

using namespace std;
using namespace cv;

/*ENUM definitions*/
struct StereoPair
{
    Mat leftImage, rightImage;
};

struct FLAGS
{
    enum NUMDISPARITY
    {
        UNCHANGED, INCREMENT, DECREMENT
    };
    enum CARSTATUS
    {
        MOBILE, REVERSE, STATIONARY
    };
    enum CLIENTDISPLAY
    {
        LEFT, RIGHT, BLOBS, DISPARITY
    };
    enum VISUALS
    {
        NEAR, FAR
    };
    enum STEREOINFO
    {
        NORMAL = 0x00,
        NO_OBJECT = 0x01,
        TEXTURELESS = 0x02,
        TOO_CLOSE = 0x03,
        ASSESSING = 0x04
    };
};

#endif /* INCLUDEFILES_H_ */
