#include "ros/ros.h"
//#include "std_msgs/Empty.h"
//#include "geometry_msgs/Twist.h"

#include "cv.h"
#include "highgui.h"

IplImage* GetThresholdedImage(IplImage* img) {
    // Convert the image into an HSV image
    IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
    cvCvtColor(img, imgHSV, CV_BGR2HSV); 
    IplImage* imgThreshed = cvCreateImage(cvGetSize(img), 8, 1); 
    cvInRangeS(imgHSV, cvScalar(20, 100, 100), cvScalar(30, 255, 255), imgThreshed); 
    cvReleaseImage(&imgHSV);
    return imgThreshed;
} 

int main() {
    // Initialize capturing live feed from the camera
    CvCapture* capture = 0; 
    capture = cvCaptureFromCAM(0); 

    // Couldn't get a device? Throw an error and quit
    if (!capture) {
        printf("Could not initialize capturing...\n");
        return -1; 
    }

    // The two windows we'll be usings
    cvNamedWindow("video"); 
    cvNamedWindow("thresh"); 
    
    // This image holds the "scribble data...
    // the tracked positions of the ball
    IplImage* imgScribble = NULL; 

    // An infinite loop
    while(true) {
        
        // Will hold a frame captured from the camera
        IplImage* frame = 0; 
        frame = cvQueryFrame(capture); 
        
        // If we couldn't grab a frame... quit
        if (!frame)
            break; 
        
        // If this is the first frame, we need to initialize it
        if (imgScribble == NULL) {
            imgScribble = cvCreateImage(cvGetSize(frame), 8, 3); 
        }

        // Holds the yellow thresholded image (yellow = white, rest = black)
        IplImage* imgYellowThresh = GetThresholdedImage(frame); 

        // Calculate the moments to estimate the position of the ball
        CvMoments *moments = (CvMoments*)malloc(sizeof(CvMoments)); 
        cvMoments(imgYellowThresh, moments, 1); 

        // The actual moment values
        double moment10 = cvGetSpatialMoment(moments, 1, 0); 
        double moment01 = cvGetSpatialMoment(moments, 0, 1); 
        double area = cvGetCentralMoment(moments, 0, 0); 

        // Holding the last and current ball positions
        static int posX = 0; 
        static int posY = 0; 

        int lastX = posX; 
        int lastY = posY; 

        posX = moment10/area;
        posY = moment01/area; 

        // Print it out for debugging purposes
        printf("position (%d, %d)\n", posX, posY); 

        // We want to draw a line only if it's a valid position
        if(lastX>0 && lastY>0 && posX>0 && posY>0) {
            // Draw a yellow line from the previous point to the current point
            cvLine(imgScribble, cvPoint(posX, posY), cvPoint(lastX, lastY), cvScalar(0,255,255), 5);
        }

        // Add the scribbling image and the frame...
        cvAdd(frame, imgScribble, frame);
        cvShowImage("thresh", imgYellowThresh);
        cvShowImage("video", frame);

        // Wait for a keypress
        int c = cvWaitKey(10);
        if(c!=-1) {
            // If pressed, break out of the loop
            break;
        }

         // Release the thresholded image+moments... we need no memory leaks.. please
        cvReleaseImage(&imgYellowThresh);
        delete moments;
    }

    // We're done using the camera. Other applications can now use it
    cvReleaseCapture(&capture);
    return 0;
}

/*
int main(int argc, char **argv) {
    ros::init(argc, argv, "thinc_main");
    ros::NodeHandle n;
    ros::Publisher takeoff_pub = n.advertise<std_msgs::Empty>("ardrone/takeoff", 5, false);
    ros::Publisher land_pub = n.advertise<std_msgs::Empty>("ardrone/land", 5, false);
    ros::Publisher reset_pub = n.advertise<std_msgs::Empty>("ardrone/reset", 5, false);
    ros::Publisher twist_pub = n.advertise<geometry_msgs::Twist>("cmd_vel", 10, false);
    ros::Rate loop_rate(10);

  ros::Duration(1.0).sleep();
    
    if(ros::ok()) {
        std_msgs::Empty takeoff_msg;
        std_msgs::Empty land_msg;
        geometry_msgs::Twist twist_msg;

        ROS_INFO("Theoretically, takeoff.\n");
        takeoff_pub.publish(takeoff_msg);
    
        ros::Duration(15.0).sleep();
        twist_msg.angular.z = 0.5;
        twist_pub.publish(twist_msg);
        ROS_INFO("Theoretically, spinning.\n");
        ros::Duration(5.0).sleep();
        twist_msg.angular.z = 0;
        twist_pub.publish(twist_msg);
        ROS_INFO("Theoretically, hovering.\n");
        ros::Duration(5.0).sleep();

        land_pub.publish(land_msg);
        ROS_INFO("Theoretically, landing.\n");
    }

    return 0;
}*/
