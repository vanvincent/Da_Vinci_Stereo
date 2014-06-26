#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "time.h"
#include <time.h>
#include "opencv2/core/core.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
// include the necessary libraries
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctime>


#define CAM1 0
#define CAM2 1
#define OVR_WIDTH 1280
#define OVR_HEIGHT 800
using namespace std;
using namespace cv;

int value=68;
int distor=5;
int offset=65;
void copyimage(IplImage*,IplImage*,IplImage*);
IplImage* barrel_pincusion_dist(IplImage* img, double Cx,double Cy,double kx,double ky);

int main(int argc, char* argv[]){
 
    CvCapture* capture = 0;
    CvCapture* capture2=0;
    capture2= cvCaptureFromCAM(CAM2);
    capture = cvCaptureFromCAM(CAM1);//CV_CAP_ANY );
    if (cvWaitKey(60) != -1) {
        cout << "Input" << endl;
    }
    if( !capture )
    {
        fprintf(stderr, "failed to initialize video capture\n");
        return -1;
    }
    
    cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH, 320 );
    cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT, 240 );

	double cumtime=0.0;
    int frames = 0;
    int frame_number = 1; 

    IplImage* frame = 0;
    IplImage* frame2 = 0;
    IplImage* frame_resized= cvCreateImage(cvSize(640,480),8,3);
    IplImage* frame_resized2= cvCreateImage(cvSize(640,480),8,3);
    IplImage* frame_stereo = cvCreateImage(cvSize(OVR_WIDTH,OVR_HEIGHT),8,3);
 
    cvNamedWindow("Video");
    cvNamedWindow("Video2");
    cvNamedWindow("SVideo");
    cvNamedWindow("resized");
    
    
    cvMoveWindow("SVideo", 0, 0);
    cvMoveWindow("Video", 0, 0);
    cvMoveWindow("resized", 400, 0);
    cvSetWindowProperty("SVideo", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    
    cvCreateTrackbar("IPD", "resized",&value, 100,NULL);
    cvCreateTrackbar("distortion factor", "resized",&distor, 100,NULL);
	cvCreateTrackbar("stereo offset", "resized",&offset, 100,NULL);

	time_t timer,timer2;
  	time(&timer);  /* get current time; same as: timer = time(NULL)  */

  	


    while(true) {
    
    	printf("=====================================\n");
    	printf("Frame: %d\n",frame_number);
    	
    		
        if ((frame = cvQueryFrame(capture)) && (frame2 = cvQueryFrame(capture2))) {
            cvSet(frame_stereo, cvScalar(0,0,0));
        	cvResize(frame,frame_resized);
        	cvResize(frame2,frame_resized2);
        	barrel_pincusion_dist(frame_resized, 320,240,0.000001*distor,0.000001*distor);
        	barrel_pincusion_dist(frame_resized2, 320,240,0.000001*distor,0.000001*distor);
        	copyimage(frame_resized,frame_resized2,frame_stereo);
            
            cvShowImage("resized",frame_resized);
            cvShowImage("SVideo",frame_stereo);
            cvShowImage("Video", frame);
            cvShowImage("Video2",frame2);
            
        }
        
        
        
        time(&timer2);  
    	cumtime = difftime(timer2,timer);
    	frames++;
    	frame_number++;
    	printf("Elapsed: %g - fps: %g\n",cumtime,frames/cumtime);
        
        
        
        if ((cvWaitKey(30) & 255)=='q') {
            break;
        }


        
    }

    cvReleaseImage(&frame);   
    cvReleaseImage(&frame_stereo);   
    cvDestroyAllWindows();
    cvReleaseCapture(&capture);
    return 0;
}

void copyimage(IplImage* frame_resized,IplImage* frame_resized2,IplImage* frame_stereo){
    // fast copy using ROI
	cvSetImageROI( frame_stereo, cvRect( value+offset, (frame_stereo->height-frame_resized->height)/2+offset/2, frame_resized->width-2*offset, frame_resized->height-offset ) );
    cvSetImageROI( frame_resized, cvRect( offset, offset/2, frame_resized->width-2*offset, frame_resized->height-offset  ) );
	cvCopy( frame_resized, frame_stereo);
	cvResetImageROI( frame_stereo);
	cvResetImageROI( frame_resized);

	cvSetImageROI( frame_stereo, cvRect( frame_resized->width-value+offset, (frame_stereo->height-frame_resized2->height)/2+offset/2, frame_resized2->width-2*offset, frame_resized2->height-offset ) );
	cvSetImageROI( frame_resized2, cvRect( offset, offset/2, frame_resized2->width-2*offset, frame_resized2->height-offset  ) );
	cvCopy( frame_resized2, frame_stereo);
	cvResetImageROI( frame_stereo);
	cvResetImageROI( frame_resized2);

//slower copy using 2 loops	
/*	
	int col, row;
	for(row=0;row<image1->height;row++)
		for(col=0;col<image1->width;col++){
		
			if(col>=0 && col<value){
				CvScalar init = cvGet2D(image1, row, col);
  				cvSet2D(image2,row+160,col+value,init);
			}
			else if(col>=image1->width-value && col<image1->width){
				CvScalar init2 = cvGet2D(image12, row, col);
  				cvSet2D(image2,row+160,col+(image1->width)-value,init2);	
			}
			else{
			
				CvScalar init = cvGet2D(image1, row, col);
  				cvSet2D(image2,row+160,col+value,init);
				CvScalar init2 = cvGet2D(image12, row, col);
  				cvSet2D(image2,row+160,col+(image1->width)-value,init2);	
			}
			
		}
*/

}


IplImage* barrel_pincusion_dist(IplImage* img, double Cx,double Cy,double kx,double ky){
    
    IplImage* mapx = cvCreateImage( cvGetSize(img), IPL_DEPTH_32F, 1 );
    IplImage* mapy = cvCreateImage( cvGetSize(img), IPL_DEPTH_32F, 1 );

    int w= img->width;
    int h= img->height;
    
    //pincusion_dis
/*
    float* pbuf = (float*)mapx->imageData;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {         
            float u= Cx+(x-Cx)*(1+kx*((x-Cx)*(x-Cx)+(y-Cy)*(y-Cy)));
            *pbuf = u;
            ++pbuf;
        }
    }

    pbuf = (float*)mapy->imageData;
    for (int y = 0;y < h; y++)
    {
        for (int x = 0; x < w; x++) 
        {
            *pbuf = Cy+(y-Cy)*(1+ky*((x-Cx)*(x-Cx)+(y-Cy)*(y-Cy)));
            ++pbuf;
        }
    }
*/

	//barrel_dis
    float* pbuf = (float*)mapx->imageData;
    for (int y = 0; y < h; y++)
    {
        int ty= y-Cy;
        for (int x = 0; x < w; x++)
        {
            int tx= x-Cx;
            int rt= tx*tx+ty*ty;

            *pbuf = (float)(tx*(1+kx*rt)+Cx);
            ++pbuf;
        }
    }

    pbuf = (float*)mapy->imageData;
    for (int y = 0;y < h; y++)
    {
        int ty= y-Cy;
        for (int x = 0; x < w; x++) 
        {
            int tx= x-Cx;
            int rt= tx*tx+ty*ty;

            *pbuf = (float)(ty*(1+ky*rt)+Cy);
            ++pbuf;
        }
    }

    IplImage* temp = cvCloneImage(img);
    cvRemap( temp, img, mapx, mapy ); 
    cvReleaseImage(&temp);
    cvReleaseImage(&mapx);
    cvReleaseImage(&mapy);

    return img;
}